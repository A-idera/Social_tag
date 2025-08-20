import serial
import serial.tools.list_ports
import threading
import re
import os
import json
import queue 
import copy
from datetime import datetime

class NrfBackend:
    def __init__(self):
        self.serial_port = None
        self.is_connected = False
        self.read_thread = None
        self.ui_update_queue = queue.Queue()
        self.data_lock = threading.Lock()
        
        # --- Simplified Data Storage ---
        self.discovered_slaves = {} #
        self.is_discovery_mode = True #
        
        self.config_file = "config.json" #
        self.config = {} #

        self.load_config() #

    def _notify_ui(self, event_name, *args):
        try:
            args_copy = copy.deepcopy(args)
            self.ui_update_queue.put((event_name, args_copy)) #
        except Exception as e:
            print(f"[ERROR] Could not deepcopy for queue: {e}")

    def load_config(self):
        with self.data_lock:
            try:
                if os.path.exists(self.config_file): #
                    with open(self.config_file, 'r', encoding='utf-8') as f:
                        self.config = json.load(f) #
                    if "user_profiles" not in self.config: self.config["user_profiles"] = {} #
                else:
                    self.config = {"user_profiles": {}} #
            except (json.JSONDecodeError, IOError):
                self.config = {"user_profiles": {}} #

    def save_config(self):
        with self.data_lock:
            try:
                with open(self.config_file, 'w', encoding='utf-8') as f:
                    json.dump(self.config, f, indent=4, ensure_ascii=False) #
                return True
            except IOError:
                return False

    def get_ports(self):
        return [p.device for p in serial.tools.list_ports.comports()] #

    def connect(self, port):
        if not port or "No ports" in port:
            self._notify_ui("status_update", "Error: No port selected", "red")
            return
        try:
            self.serial_port = serial.Serial(port, 115200, timeout=1) #
            self.is_connected = True #
            self.read_thread = threading.Thread(target=self.read_from_port, daemon=True) #
            self.read_thread.start() #
            self._notify_ui("connection_status", True, f"Connected to {port}")
        except serial.SerialException as e:
            self._notify_ui("status_update", f"Error: {e}", "red")

    def disconnect(self):
        self.is_connected = False #
        if self.read_thread and self.read_thread.is_alive():
            self.read_thread.join(timeout=0.2)
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        with self.data_lock: self.discovered_slaves.clear() #
        self._notify_ui("connection_status", False, "Disconnected")

    def read_from_port(self):
        while self.is_connected:
            try:
                line = self.serial_port.readline().decode('utf-8', errors='ignore').strip() #
                if line:
                    self._notify_ui("raw_log", line)
                    self.process_log_line(line)
            except (serial.SerialException, TypeError):
                if self.is_connected: self._notify_ui("connection_status", False, "Serial port disconnected")
                break
    
    def process_log_line(self, line):
        # This function is now much simpler. It only cares about system status and device list.
        if "New slave joined" in line:
            match = re.search(r"ID: (\d+)", line)
            if match:
                device_id = int(match.group(1))
                with self.data_lock:
                    if device_id not in self.discovered_slaves:
                        self.discovered_slaves[device_id] = {} #
                        self._notify_ui("slave_update", self.discovered_slaves, self.config.get("user_profiles", {}))
        
        elif "Device #" in line and "removed" in line:
            match = re.search(r"Device #(\d+) removed", line)
            if match:
                device_id = int(match.group(1))
                with self.data_lock:
                    if device_id in self.discovered_slaves:
                        del self.discovered_slaves[device_id] #
                        self._notify_ui("slave_update", self.discovered_slaves, self.config.get("user_profiles", {}))
        
        elif "System now in Discovery Mode" in line:
            with self.data_lock: self.is_discovery_mode = True #
            self._notify_ui("mode_update", True)

        elif "System switching to Idle Mode" in line or "Ready for UI commands" in line:
            with self.data_lock: self.is_discovery_mode = False #
            self._notify_ui("mode_update", False)

    def _send_command(self, command, log=True):
        if self.is_connected:
            full_command = f"{command}\n"
            self.serial_port.write(full_command.encode('utf-8')) #
            if log: self._notify_ui("raw_log", f"[UI CMD] -> {command}")

    # --- Game Control Functions ---

    def start_pairing(self):
        self._send_command("*RANDOM_01#") #

    def send_stop_command(self):
        self._send_command("*STOP_ALL#") #
    
    def send_scenario1_start(self):
        self._send_command("*SCENARIO1_START#")
    
    def send_scenario2_start(self):
        self._send_command("*SCENARIO2_START#")
    
    # --- System Control Functions ---

    def toggle_discovery_mode(self):
        with self.data_lock: is_disc = self.is_discovery_mode #
        if is_disc: self._send_command("*DISCOVERY_00#") #
        else: self._send_command("*DISCOVERY_01#") #
        # UI stateはマスターのログで更新されるが、ユーザーの指示通り継続探索が既定なので何もしない

    def remove_slave(self, device_id):
        if self.is_connected and device_id is not None:
            self._send_command(f"*REMOVEJOIN_{device_id}#") #

    # --- Profile/Name Registration ---
    def send_name(self, device_id, name):
        if device_id is None or not name:
            return
        self._send_command(f"*NAME_{device_id}={name}#")