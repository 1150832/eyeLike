import time
import json
import statistics
import paho.mqtt.client as mqtt

BROKER = "localhost"
PORT = 1883
TOPIC = "#"  # Subscribes to all topics

payload_sizes = []
timestamps = []
sample_payload = None

def on_connect(client, userdata, flags, rc):
    print(f"[+] Conectado ao broker ({BROKER}:{PORT})")
    print("[*] A aguardar mensagens do eyeLike...")
    client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    global sample_payload
    payload_str = msg.payload.decode('utf-8', errors='ignore').strip()

    # ---> ACRESCENTA ESTAS DUAS LINHAS AQUI <---
    if payload_str in ["online", "offline"] or "discovery" in msg.topic:
        return

    now = time.time()
    timestamps.append(now)
    payload_sizes.append(len(msg.payload))
    
    if sample_payload is None:
        try:
            sample_payload = json.loads(payload_str)
        except Exception:
            sample_payload = payload_str

    if len(payload_sizes) >= 100:
        client.disconnect()

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(BROKER, PORT, 60)
    client.loop_forever()
except Exception as e:
    print(f"[ERRO] {e}")
    exit(1)

deltas = [t2 - t1 for t1, t2 in zip(timestamps[:-1], timestamps[1:])]
fps_real = 1.0 / statistics.mean(deltas) if deltas else 0
jitter_ms = statistics.stdev(deltas) * 1000 if len(deltas) > 1 else 0

print("\n" + "="*50)
print("       RELATÓRIO DE TELEMETRIA MQTT (EIXO 3)")
print("="*50)
print(f"Total de Mensagens Analisadas : {len(payload_sizes)}")
print(f"Débito Efetivo (Throughput)   : {fps_real:.2f} msg/s (Hz)")
print(f"Intervalo Médio Entre Envios  : {statistics.mean(deltas)*1000:.2f} ms")
print(f"Jitter Temporal (StdDev)      : {jitter_ms:.2f} ms")
print(f"Tamanho Médio do Payload      : {statistics.mean(payload_sizes):.1f} bytes")
print(f"Tamanho Mínimo / Máximo       : {min(payload_sizes)} B / {max(payload_sizes)} B")
print("-" * 50)
print("Amostra da Estrutura JSON:")
print(json.dumps(sample_payload, indent=2) if isinstance(sample_payload, dict) else sample_payload)
print("="*50 + "\n")