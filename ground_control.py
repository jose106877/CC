#!/usr/bin/env python3
"""
Ground Control - Visualização de dados da Nave-Mãe (OTIMIZADO)
Acede à API de Observação com tratamento de timeouts
"""

import requests
import json
import time
import os
import sys
from datetime import datetime
from urllib3.exceptions import InsecureRequestWarning

requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

API_URL = "http://localhost:8080/api"
REQUEST_TIMEOUT = 5  # Aumentado para 5 segundos
RETRY_DELAY = 0.5   # Delay entre retries

class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    END = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def clear_screen():
    os.system('clear' if os.name == 'posix' else 'cls')

def print_header():
    print(f"{Colors.BOLD}{Colors.CYAN}")
    print("╔════════════════════════════════════════════════════════════════════╗")
    print("║                    🌐 GROUND CONTROL - Nave-Mãe                    ║")
    print("║                      Sistema de Monitorização                      ║")
    print("╚════════════════════════════════════════════════════════════════════╝")
    print(f"{Colors.END}")

def get_api_data_with_retry(endpoint, max_retries=2):
    """Fetch data from API com retry logic"""
    for attempt in range(max_retries):
        try:
            response = requests.get(
                f"{API_URL}{endpoint}",
                timeout=REQUEST_TIMEOUT,
                allow_redirects=False
            )
            if response.status_code == 200:
                return response.json()
            elif response.status_code == 404:
                return None
        except requests.exceptions.Timeout:
            if attempt < max_retries - 1:
                time.sleep(RETRY_DELAY)
                continue
        except requests.exceptions.ConnectionError:
            if attempt < max_retries - 1:
                time.sleep(RETRY_DELAY)
                continue
        except Exception:
            if attempt < max_retries - 1:
                time.sleep(RETRY_DELAY)
                continue
    
    return None

def get_system_status():
    return get_api_data_with_retry("/system/status")

def get_rovers_list():
    return get_api_data_with_retry("/rovers")

def get_missions_list():
    return get_api_data_with_retry("/missions")

def get_telemetry_latest():
    return get_api_data_with_retry("/telemetry/latest")

def print_system_status(data):
    if not data or 'system' not in data:
        print(f"{Colors.RED}❌ Sem dados do sistema{Colors.END}\n")
        return False
    
    sys_data = data['system']
    print(f"{Colors.BOLD}📊 STATUS DO SISTEMA{Colors.END}")
    print(f"  Timestamp: {datetime.fromtimestamp(sys_data['timestamp']).strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"  Rovers: {sys_data['rovers']['active']}/{sys_data['rovers']['total']} ativos")
    print(f"  Missões: {sys_data['missions']['in_progress']} em progresso, {sys_data['missions']['completed']} concluídas")
    print(f"  Telemetria: {sys_data['telemetry']['active']}/{sys_data['telemetry']['sessions']} sessões ativas\n")
    return True

def print_rovers(data):
    if not data or 'rovers' not in data:
        print(f"{Colors.YELLOW}⚠ Aguardando dados de rovers...{Colors.END}\n")
        return False
    
    rovers = data.get('rovers', [])
    if not rovers:
        print(f"{Colors.YELLOW}⚠ Nenhum rover conectado{Colors.END}\n")
        return False
    
    print(f"{Colors.BOLD}🤖 ROVERS CONECTADOS ({len(rovers)}){Colors.END}")
    print(f"{'ID':<12} {'Status':<14} {'Bateria':<12} {'Progresso':<14} {'Missão':<15}")
    print("─" * 70)
    
    for rover in rovers:
        status = f"{Colors.GREEN}✓ Ativo{Colors.END}" if rover['status'] == 'active' else f"{Colors.RED}✗ Inativo{Colors.END}"
        battery = f"{Colors.GREEN}{rover['battery']}%{Colors.END}" if rover['battery'] > 30 else f"{Colors.RED}{rover['battery']}%{Colors.END}"
        
        print(f"{rover['id']:<12} {status:<14} {battery:<12} {rover['progress']:>3}%           {rover['mission_id']:<15}")
    print()
    return True

def print_missions(data):
    if not data or 'missions' not in data:
        print(f"{Colors.YELLOW}⚠ Aguardando dados de missões...{Colors.END}\n")
        return False
    
    missions = data.get('missions', [])
    if not missions:
        print(f"{Colors.YELLOW}⚠ Nenhuma missão criada{Colors.END}\n")
        return False
    
    print(f"{Colors.BOLD}🎯 MISSÕES (Total: {len(missions)}){Colors.END}")
    print(f"{'ID':<10} {'Rover':<10} {'Tarefa':<20} {'Progresso':<14} {'Status':<14}")
    print("─" * 70)
    
    for mission in missions:
        status_color = Colors.GREEN if mission['status'] == 'completed' else Colors.YELLOW
        status_text = "✅ Concluída" if mission['status'] == 'completed' else "⏳ Em Progresso"
        
        print(f"{mission['id']:<10} {mission['rover_id']:<10} {mission['task_type']:<20} {mission['progress']:>3}%          {status_color}{status_text}{Colors.END}")
    print()
    return True

def print_telemetry(data):
    if not data or 'telemetry' not in data:
        print(f"{Colors.YELLOW}⚠ Aguardando dados de telemetria...{Colors.END}\n")
        return False
    
    telemetry = data.get('telemetry', [])
    if not telemetry:
        print(f"{Colors.YELLOW}⚠ Nenhuma telemetria recebida{Colors.END}\n")
        return False
    
    print(f"{Colors.BOLD}📡 TELEMETRIA (Total: {len(telemetry)} rovers){Colors.END}")
    print(f"{'Rover':<10} {'Posição':<20} {'Bateria':<10} {'Temp':<10} {'Sinal':<10} {'Estado':<15}")
    print("─" * 75)
    
    for telem in telemetry:
        pos = f"({telem['position']['x']:.1f}, {telem['position']['y']:.1f})"
        state_colors = {
            'idle': Colors.BLUE,
            'in_mission': Colors.YELLOW,
            'returning': Colors.CYAN,
            'charging': Colors.GREEN,
            'error': Colors.RED
        }
        state_color = state_colors.get(telem['state'], '')
        
        battery_color = Colors.GREEN if telem['battery'] > 30 else Colors.RED
        
        print(f"{telem['rover_id']:<10} {pos:<20} {battery_color}{telem['battery']}%{Colors.END:<8} {telem['temperature']:>5.1f}°C {telem['signal_strength']:>3}%      {state_color}{telem['state']}{Colors.END}")
    print()
    return True

def show_menu():
    print(f"\n{Colors.BOLD}{Colors.CYAN}🎮 OPÇÕES:{Colors.END}")
    print("  1 - Ver status do sistema")
    print("  2 - Ver rovers")
    print("  3 - Ver missões")
    print("  4 - Ver telemetria")
    print("  5 - Ver tudo (dashboard)")
    print("  6 - Atualizar ao vivo (10s)")
    print("  0 - Sair")
    print()

def main():
    print(f"{Colors.BOLD}Ligando ao Ground Control...{Colors.END}")
    time.sleep(1)
    
    # Testar conexão com retry
    connected = False
    for attempt in range(3):
        try:
            response = requests.get(f"{API_URL}/system/status", timeout=REQUEST_TIMEOUT)
            if response.status_code == 200:
                connected = True
                break
        except:
            if attempt < 2:
                print(f"{Colors.YELLOW}⚠ Tentativa {attempt + 1}/3...{Colors.END}")
                time.sleep(1)
    
    if not connected:
        print(f"{Colors.RED}❌ Não consegue conectar à API em {API_URL}{Colors.END}")
        print("   Certifique-se que a Nave-Mãe está a correr:")
        print("   Terminal 1: make run-server")
        sys.exit(1)
    
    print(f"{Colors.GREEN}✓ Conectado com sucesso!{Colors.END}\n")
    time.sleep(1)
    
    # Loop interativo
    while True:
        clear_screen()
        print_header()
        print(f"Atualização: {datetime.now().strftime('%H:%M:%S')}\n")
        
        show_menu()
        choice = input("Escolha uma opção: ").strip()
        
        if choice == '0':
            print(f"{Colors.GREEN}✓ Ground Control encerrado{Colors.END}")
            break
        
        elif choice == '1':
            clear_screen()
            print_header()
            data = get_system_status()
            print_system_status(data)
            input("Pressione Enter para continuar...")
        
        elif choice == '2':
            clear_screen()
            print_header()
            data = get_rovers_list()
            print_rovers(data)
            input("Pressione Enter para continuar...")
        
        elif choice == '3':
            clear_screen()
            print_header()
            data = get_missions_list()
            print_missions(data)
            input("Pressione Enter para continuar...")
        
        elif choice == '4':
            clear_screen()
            print_header()
            data = get_telemetry_latest()
            print_telemetry(data)
            input("Pressione Enter para continuar...")
        
        elif choice == '5':
            clear_screen()
            print_header()
            print(f"Atualização: {datetime.now().strftime('%H:%M:%S')}\n")
            print_system_status(get_system_status())
            print_rovers(get_rovers_list())
            print_missions(get_missions_list())
            print_telemetry(get_telemetry_latest())
            input("Pressione Enter para continuar...")
        
        elif choice == '6':
            live_monitoring_short()
        
        else:
            print(f"{Colors.RED}❌ Opção inválida{Colors.END}")
            time.sleep(1)

def live_monitoring_short():
    """Modo de atualização rápida - 10 segundos"""
    print(f"{Colors.GREEN}✓ Atualizando ao vivo por 10 segundos (Ctrl+C para parar){Colors.END}\n")
    
    start_time = time.time()
    try:
        while True:
            clear_screen()
            print_header()
            elapsed = int(time.time() - start_time)
            print(f"🔄 Modo ao vivo: {elapsed}/10s | {datetime.now().strftime('%H:%M:%S')}\n")
            
            sys_ok = print_system_status(get_system_status())
            rov_ok = print_rovers(get_rovers_list())
            mis_ok = print_missions(get_missions_list())
            tel_ok = print_telemetry(get_telemetry_latest())
            
            if not (sys_ok or rov_ok or mis_ok or tel_ok):
                print(f"{Colors.RED}⚠ Aguardando dados da Nave-Mãe...{Colors.END}")
            
            time.sleep(2)
    except KeyboardInterrupt:
        print(f"\n{Colors.GREEN}✓ Atualização ao vivo parada{Colors.END}")

def live_monitoring():
    """Modo contínuo de monitorização (opcional)"""
    print(f"{Colors.GREEN}✓ Iniciando monitorização contínua (Ctrl+C para parar){Colors.END}\n")
    
    try:
        while True:
            clear_screen()
            print_header()
            print(f"🔄 Atualização: {datetime.now().strftime('%H:%M:%S')}\n")
            
            print_system_status(get_system_status())
            print_rovers(get_rovers_list())
            print_missions(get_missions_list())
            print_telemetry(get_telemetry_latest())
            
            time.sleep(3)
    except KeyboardInterrupt:
        print(f"\n{Colors.GREEN}✓ Monitorização encerrada{Colors.END}")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--live":
        live_monitoring()
    else:
        main()