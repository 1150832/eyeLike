import paho.mqtt.client as mqtt
import json
import numpy as np
import cv2
import csv
import os
from datetime import datetime
from collections import deque, defaultdict

# ==========================================
# CONFIGURAÇÕES E REGISTO
# ==========================================
BROKER_ADDRESS = "localhost"
PORT = 1883
TOPIC_PATTERN = "eyetracker/coordinates/+/eyes"
TIME_TOLERANCE_MS = 50 

LOG_DIR = os.path.join("test_reports", "telemetry")
os.makedirs(LOG_DIR, exist_ok=True)

CSV_FILE = os.path.join(LOG_DIR, "3d_telemetry_log.csv")

if not os.path.exists(CSV_FILE):
    with open(CSV_FILE, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            "Timestamp_System", "Delta_T_ms", "Sensor_A", "Sensor_B",
            "Left_X", "Left_Y", "Left_Z",
            "Right_X", "Right_Y", "Right_Z"
        ])

sensor_buffers = defaultdict(lambda: deque(maxlen=30))

# ==========================================
# GEOMETRIA ESTÉREO
# ==========================================
def load_generic_projection_matrices():
    focal_length = 800.0
    cx, cy = 640.0, 360.0
    
    P1 = np.array([
        [focal_length, 0, cx, 0],
        [0, focal_length, cy, 0],
        [0, 0, 1, 0]
    ], dtype=np.float64)
    
    Tx = -190.0 
    P2 = np.array([
        [focal_length, 0, cx, focal_length * Tx],
        [0, focal_length, cy, 0],
        [0, 0, 1, 0]
    ], dtype=np.float64)
    
    return P1, P2

P1, P2 = load_generic_projection_matrices()

def triangulate_point(pt_a, pt_b, P1, P2):
    pt_a_np = np.array([[pt_a['x']], [pt_a['y']]], dtype=np.float64)
    pt_b_np = np.array([[pt_b['x']], [pt_b['y']]], dtype=np.float64)
    
    homog_4d = cv2.triangulatePoints(P1, P2, pt_a_np, pt_b_np)
    cartesian_3d = homog_4d[:3] / homog_4d[3]
    
    return {
        "x": float(cartesian_3d[0][0]), 
        "y": float(cartesian_3d[1][0]), 
        "z": float(cartesian_3d[2][0])
    }

# ==========================================
# MOTOR DE FUSÃO E GRAVAÇÃO
# ==========================================
def process_fusion(id_a, data_a, id_b, data_b):
    delta_t = abs(data_a['timestamp'] - data_b['timestamp'])
    
    try:
        left_3d = triangulate_point(data_a['left'], data_b['left'], P1, P2)
        right_3d = triangulate_point(data_a['right'], data_b['right'], P1, P2)
        
        print(f"[FUSÃO 3D FIXA] Par: [{id_a} (P1) <-> {id_b} (P2)] | Delta T: {delta_t:.2f} ms")
        print(f" -> Esquerdo: X={left_3d['x']:.1f}, Y={left_3d['y']:.1f}, Z={left_3d['z']:.1f}")
        print(f" -> Direito : X={right_3d['x']:.1f}, Y={right_3d['y']:.1f}, Z={right_3d['z']:.1f}")
        print("-" * 65)

        with open(CSV_FILE, mode='a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                datetime.now().isoformat(), f"{delta_t:.2f}", id_a, id_b,
                f"{left_3d['x']:.2f}", f"{left_3d['y']:.2f}", f"{left_3d['z']:.2f}",
                f"{right_3d['x']:.2f}", f"{right_3d['y']:.2f}", f"{right_3d['z']:.2f}"
            ])

    except Exception as e:
        print(f"[ERRO DE CÁLCULO 3D]: {e}")

def on_message(client, userdata, message):
    try:
        topic_tokens = message.topic.split('/')
        if len(topic_tokens) < 3:
            return
        sensor_id = topic_tokens[2]

        payload = json.loads(message.payload.decode("utf-8"))
        if 'left' not in payload or 'right' not in payload or 'timestamp' not in payload:
            return

        sensor_buffers[sensor_id].append(payload)

        active_sensors = list(sensor_buffers.keys())
        if len(active_sensors) >= 2:
            current_packet = sensor_buffers[sensor_id][-1]
            for peer_sensor in active_sensors:
                if peer_sensor == sensor_id:
                    continue
                for peer_packet in list(sensor_buffers[peer_sensor]):
                    time_diff = abs(current_packet['timestamp'] - peer_packet['timestamp'])
                    if time_diff <= TIME_TOLERANCE_MS:
                        # Fix de atribuição determinística das câmaras:
                        cam1_id, cam2_id = sorted([sensor_id, peer_sensor])
                        pkt_cam1 = current_packet if sensor_id == cam1_id else peer_packet
                        pkt_cam2 = peer_packet if sensor_id == cam1_id else current_packet

                        process_fusion(cam1_id, pkt_cam1, cam2_id, pkt_cam2)

                        sensor_buffers[sensor_id].clear()
                        sensor_buffers[peer_sensor].clear()
                        break
    except Exception as e:
        print(f"[ERRO NO MQTT]: {e}")

print("A inicializar Motor Central de Fusão 3D (com Atribuição Determinística de Nós)...")
client = mqtt.Client()
client.on_message = on_message
client.connect(BROKER_ADDRESS, PORT)
client.subscribe(TOPIC_PATTERN, 0)
print(f"A gravar dados em '{CSV_FILE}'...")
client.loop_forever()