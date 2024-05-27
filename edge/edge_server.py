import paho.mqtt.client as mqtt
from pydantic import BaseModel

# MQTT Callbacks
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe("/status")
        client.subscribe("/optimization_result")
        client.subscribe("/optimal_rate result")
        client.subscribe("/max_sampling_rate_observed_result")
        client.subscribe("/avarage_analysis_result")
        client.subscribe("/task_time")
        client.subscribe("/monitoring")
       
        
    else:
        print("Connect failed with code", rc)


def on_message(client, userdata, msg):
    print(f"Message {msg.payload}") #('utf-8')

client = mqtt.Client()

# Set TLS/SSL parameters
# client.tls_set(
#     ca_certs="C:\\Users\\robyy\\IoT\\Analyzing_signal_copy\\edge-server\\broker\\certs\\ca_cert.pem",
#     certfile="C:\\Users\\robyy\\IoT\\Analyzing_signal_copy\\edge-server\\broker\\certs\\client_cert.pem",
#     keyfile="C:\\Users\\robyy\\IoT\\Analyzing_signal_copy\\edge-server\\broker\\certs\\client_key.pem",
#     tls_version=mqtt.ssl.PROTOCOL_TLS,
# )

# Set the callbacks
client.on_connect = on_connect
client.on_message = on_message

# Connect to the broker using secure port
client.connect("localhost", 1883, 60)
client.loop_start()

# Keep the script running
while True:
    pass