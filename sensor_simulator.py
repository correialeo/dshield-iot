import requests
import time
import random
import json
from datetime import datetime
import threading

API_URL = "http://localhost:5046/api/DeviceData"

SENSORS = {
    6: {
        "type": "WaterLevelSensor",
        "name": "Sensor de Nível de Água #6",
        "unit": "cm",
        "normal_range": (0, 100),     
        "alert_range": (100, 200),    
        "interval": 3.0 
    },
    7: {
        "type": "SmokeSensor", 
        "name": "Sensor de Fumaça #7",
        "unit": "ppm",
        "normal_range": (5, 200),
        "alert_range": (200, 1000),
        "interval": 2.0
    }
}

class SensorSimulator:
    def __init__(self):
        self.running = False
        self.simulation_mode = "normal"
        self.threads = []
        
    def send_to_api(self, device_id, value):
        """Envia os dados para a API"""
        try:
            payload = {
                "deviceId": device_id,
                "value": round(value, 1)
            }
            
            headers = {
                "Content-Type": "application/json",
                "Accept": "application/json"
            }
            
            print(f"📤 [API] Enviando: DeviceId={device_id}, Value={value:.1f}")
            
            response = requests.post(API_URL, json=payload, headers=headers, timeout=10)
            
            if response.status_code == 200:
                print(f"✅ [API] Sucesso ({response.status_code}): {response.text}")
            else:
                print(f"❌ [API] Erro ({response.status_code}): {response.text}")
                
        except requests.exceptions.RequestException as e:
            print(f"❌ [API] Erro na conexão: {e}")
        except Exception as e:
            print(f"❌ [API] Erro inesperado: {e}")
    
    def generate_water_level(self, mode="normal"):
        """Gera valores para sensor de nível de água"""
        sensor = SENSORS[6]
        
        if mode == "normal":
            base_value = random.uniform(*sensor["normal_range"])
        elif mode == "alert":
            base_value = random.uniform(*sensor["alert_range"])
        else:  
            rand = random.random()
            if rand < 0.7:
                base_value = random.uniform(*sensor["normal_range"])
            else:
                base_value = random.uniform(*sensor["alert_range"])
        
        noise = random.uniform(-1.0, 1.0)
        value = max(0, min(200, base_value + noise))
        
        return value
    
    def generate_smoke_level(self, mode="normal"):
        """Gera valores para sensor de fumaça"""
        sensor = SENSORS[7]
        
        if mode == "normal":
            base_value = random.uniform(*sensor["normal_range"])
        elif mode == "alert":
            base_value = random.uniform(*sensor["alert_range"])
        else:
            rand = random.random()
            if rand < 0.7:
                base_value = random.uniform(*sensor["normal_range"])
            else:
                base_value = random.uniform(*sensor["alert_range"])
        
        noise = random.uniform(-2.0, 2.0)
        value = max(5, min(1000, base_value + noise))
        
        return value
    
    def get_alert_level(self, device_id, value):
        """Determina o nível de alerta baseado no valor"""
        if device_id == 6:
            if value > 100:
                return "🚨 ALERTA: Enchente!"
            else:
                return "✅ Normal"
        
        elif device_id == 7:
            if value > 200:
                return "🚨 ALERTA: Fumaça detectada!"
            else:
                return "✅ Normal"
    
    def simulate_water_sensor(self):
        """Thread para simular sensor de água"""
        device_id = 6
        sensor = SENSORS[device_id]
        
        while self.running:
            try:
                value = self.generate_water_level(self.simulation_mode)
                alert = self.get_alert_level(device_id, value)
                
                print(f"\n🌊 [{sensor['name']}] Nível: {value:.1f} {sensor['unit']} - {alert}")
                
                self.send_to_api(device_id, value)
                
                time.sleep(sensor["interval"])
                
            except Exception as e:
                print(f"❌ Erro no sensor de água: {e}")
                time.sleep(1)
    
    def simulate_smoke_sensor(self):
        """Thread para simular sensor de fumaça"""
        device_id = 7
        sensor = SENSORS[device_id]
        
        while self.running:
            try:
                value = self.generate_smoke_level(self.simulation_mode)
                alert = self.get_alert_level(device_id, value)
                
                print(f"\n🔥 [{sensor['name']}] Fumaça: {value:.1f} {sensor['unit']} - {alert}")
                
                self.send_to_api(device_id, value)
                
                time.sleep(sensor["interval"])
                
            except Exception as e:
                print(f"❌ Erro no sensor de fumaça: {e}")
                time.sleep(1)
    
    def start_simulation(self, mode="normal"):
        """Inicia a simulação"""
        if self.running:
            print("❌ Simulação já está rodando!")
            return
        
        self.simulation_mode = mode
        self.running = True
        
        print(f"\n🚀 Iniciando simulação em modo: {mode.upper()}")
        print("=" * 50)
        print("Sensores configurados:")
        for device_id, sensor in SENSORS.items():
            print(f"  - ID {device_id}: {sensor['name']} (intervalo: {sensor['interval']}s)")
        print("=" * 50)
        
        water_thread = threading.Thread(target=self.simulate_water_sensor, daemon=True)
        smoke_thread = threading.Thread(target=self.simulate_smoke_sensor, daemon=True)
        
        self.threads = [water_thread, smoke_thread]
        
        water_thread.start()
        smoke_thread.start()
        
        print("✅ Simulação iniciada! Pressione Ctrl+C para parar.")
    
    def stop_simulation(self):
        """Para a simulação"""
        self.running = False
        print("\n🛑 Parando simulação...")
        time.sleep(2)
        print("✅ Simulação parada.")

def main():
    simulator = SensorSimulator()
    
    print("🔧 SIMULADOR DE SENSORES IoT")
    print("=" * 40)
    print("API URL:", API_URL)
    print("Sensores disponíveis:")
    for device_id, sensor in SENSORS.items():
        print(f"  - ID {device_id}: {sensor['name']}")
    print("=" * 40)
    
    while True:
        print("\nModos disponíveis:")
        print("1. Normal (sem alertas)")
        print("2. Alert (com alertas)")
        print("3. Mixed (70% normal, 30% alert)")
        print("4. Sair")
        
        try:
            choice = input("\nEscolha o modo (1-4): ").strip()
            
            if choice == "1":
                simulator.start_simulation("normal")
            elif choice == "2":
                simulator.start_simulation("alert")
            elif choice == "3":
                simulator.start_simulation("mixed")
            elif choice == "4":
                print("👋 Saindo...")
                break
            else:
                print("❌ Opção inválida!")
                continue
            
            try:
                while simulator.running:
                    time.sleep(1)
            except KeyboardInterrupt:
                simulator.stop_simulation()
                
        except KeyboardInterrupt:
            simulator.stop_simulation()
            break
        except Exception as e:
            print(f"❌ Erro: {e}")

if __name__ == "__main__":
    main()