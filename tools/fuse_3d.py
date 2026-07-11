import json
import argparse
import numpy as np
import cv2
import os

def load_json(filepath):
    if not os.path.exists(filepath):
        print(f"Erro: Ficheiro não encontrado - {filepath}")
        return None
    with open(filepath, 'r') as f:
        return json.load(f)

def load_stereo_calibration(xml_path):
    """
    No futuro, esta função vai ler o calibration_stereo.xml gerado pelo teu C++.
    Por agora, cria matrizes de Projeção (P1, P2) dummy (falsas) para o código poder correr
    e demonstrar a arquitetura sem dar erro.
    """
    print(f"A carregar calibração de: {xml_path}")
    # TODO: Substituir por cv2.FileStorage quando tiveres o XML real
    
    # Matrizes dummy (assumindo câmaras idênticas, separadas horizontalmente por 190mm)
    focal_length = 800.0
    cx, cy = 640.0, 360.0
    
    # P1 (Câmara Esquerda - Origem 0,0,0)
    P1 = np.array([
        [focal_length, 0, cx, 0],
        [0, focal_length, cy, 0],
        [0, 0, 1, 0]
    ], dtype=np.float64)
    
    # P2 (Câmara Direita - Deslocada 190mm no eixo X)
    Tx = -190.0 # 19cm
    P2 = np.array([
        [focal_length, 0, cx, focal_length * Tx],
        [0, focal_length, cy, 0],
        [0, 0, 1, 0]
    ], dtype=np.float64)
    
    return P1, P2

def triangulate_point(pt1, pt2, P1, P2):
    """Triangula um único par de pontos 2D (x,y) para um ponto 3D (X,Y,Z)"""
    # Converter para formato que o OpenCV espera: shape (2, N)
    pt1_np = np.array([[pt1['x']], [pt1['y']]], dtype=np.float64)
    pt2_np = np.array([[pt2['x']], [pt2['y']]], dtype=np.float64)
    
    # Triangulação
    pt3d_homog = cv2.triangulatePoints(P1, P2, pt1_np, pt2_np)
    
    # Converter de coordenadas homogéneas (X,Y,Z,W) para cartesianas (X,Y,Z)
    pt3d = pt3d_homog[:3] / pt3d_homog[3]
    return {"x": float(pt3d[0][0]), "y": float(pt3d[1][0]), "z": float(pt3d[2][0])}

def main():
    parser = argparse.ArgumentParser(description="Módulo de Fusão Estéreo 3D")
    parser.add_argument("--left", required=True, help="Caminho para o JSON da câmara esquerda (ex: iPhone)")
    parser.add_argument("--right", required=True, help="Caminho para o JSON da câmara direita (ex: Mac)")
    parser.add_argument("--offset", type=int, required=True, help="Offset de frames: (Frame Direita) - (Frame Esquerda)")
    parser.add_argument("--calib", default="calibration_stereo.xml", help="Ficheiro XML de calibração")
    parser.add_argument("--out", default="output_3d.json", help="Nome do ficheiro JSON de saída")
    
    args = parser.parse_args()

    # 1. Carregar Dados
    data_left = load_json(args.left)
    data_right = load_json(args.right)
    if not data_left or not data_right: return

    # Converter listas de anotações em Dicionários rápidos indexados pelo frame_number
    ann_left = {item['frame_number']: item for item in data_left['annotations']}
    ann_right = {item['frame_number']: item for item in data_right['annotations']}

    # 2. Carregar Matrizes de Calibração
    P1, P2 = load_stereo_calibration(args.calib)

    # 3. Fusão e Emparelhamento Temporal
    fused_results = []
    frames_esquerdos = sorted(ann_left.keys())

    print(f"\nA iniciar Fusão 3D (Offset: {args.offset} frames)...")
    
    for f_left in frames_esquerdos:
        f_right = f_left + args.offset
        
        # Verifica se existe um frame correspondente na direita
        if f_right in ann_right:
            left_data = ann_left[f_left]
            right_data = ann_right[f_right]
            
            # Só podemos triangular se os olhos não estiverem fechados ou ocultos em AMBAS as câmaras
            if left_data['eyes_closed'] or right_data['eyes_closed']:
                continue
            if left_data['left_missing'] or right_data['left_missing']:
                continue # Faltam dados do olho esquerdo
            if left_data['right_missing'] or right_data['right_missing']:
                continue # Faltam dados do olho direito

            # Triangulação 3D (Olho Esquerdo e Direito)
            left_eye_3d = triangulate_point(left_data['left_eye'], right_data['left_eye'], P1, P2)
            right_eye_3d = triangulate_point(left_data['right_eye'], right_data['right_eye'], P1, P2)

            fused_results.append({
                "synced_frame_id": f_left, # Usamos o frame esquerdo como referência temporal primária
                "left_camera_frame": f_left,
                "right_camera_frame": f_right,
                "timestamp_relativo": left_data['timestamp'],
                "left_eye_3d": left_eye_3d,
                "right_eye_3d": right_eye_3d
            })

    # 4. Gravar Resultado Final
    output_data = {
        "metadata": {
            "source_left": args.left,
            "source_right": args.right,
            "frame_offset": args.offset,
            "total_3d_frames_generated": len(fused_results)
        },
        "trajectory_3d": fused_results
    }

    with open(args.out, 'w') as f:
        json.dump(output_data, f, indent=4)

    print(f"Sucesso! {len(fused_results)} frames triangulados e guardados em '{args.out}'.")

if __name__ == "__main__":
    main()