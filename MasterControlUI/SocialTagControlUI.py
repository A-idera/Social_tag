import tkinter as tk
import customtkinter as ctk
import queue
from nrf_backend import NrfBackend

class SocialTagControlUI(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("Social Tag Control UI - v10.0 Simplified") #
        self.geometry("1024x768")
        self.backend = NrfBackend() #
        self.device_buttons = {} #
        self.profile_entries = {} #
        self.currently_selected_id = None #
        
        self.setup_ui() #
        
        # Simplified callback dictionary
        self.callbacks = {
            "status_update": self.on_status_update, #
            "connection_status": self.on_connection_status_update, #
            "raw_log": self.on_raw_log, #
            "slave_update": self.on_slave_update, #
            "mode_update": self.on_mode_update, #
        }
        self.scan_ports() #
        self.protocol("WM_DELETE_WINDOW", self.on_closing) #
        self._process_ui_queue() #

    def _process_ui_queue(self):
        try:
            while not self.backend.ui_update_queue.empty(): #
                event_name, args = self.backend.ui_update_queue.get_nowait() #
                if event_name in self.callbacks:
                    self.callbacks[event_name](*args)
        except queue.Empty:
            pass
        finally:
            self.after(100, self._process_ui_queue) #

    def setup_ui(self):
        self.grid_columnconfigure(0, weight=1); self.grid_rowconfigure(2, weight=1)
        
        # --- Connection Frame ---
        conn_frame = ctk.CTkFrame(self); conn_frame.grid(row=0, column=0, padx=10, pady=10, sticky="ew")
        ctk.CTkLabel(conn_frame, text="Port:").pack(side="left", padx=(10, 5)); self.port_menu = ctk.CTkComboBox(conn_frame, values=[""], width=200); self.port_menu.pack(side="left", padx=5) #
        self.refresh_button = ctk.CTkButton(conn_frame, text="Refresh", width=80, command=self.scan_ports); self.refresh_button.pack(side="left", padx=5) #
        self.connect_button = ctk.CTkButton(conn_frame, text="Connect", width=100, command=self.handle_connect_button); self.connect_button.pack(side="left", padx=(10, 5)) #
        self.status_label = ctk.CTkLabel(conn_frame, text="Disconnected", text_color="gray"); self.status_label.pack(side="right", padx=10) #
        
        # --- Control Frame (Simplified) ---
        control_frame = ctk.CTkFrame(self); control_frame.grid(row=1, column=0, padx=10, pady=0, sticky="ew") #
        control_frame.grid_columnconfigure((0,1), weight=1) 
        self.pairing_button = ctk.CTkButton(control_frame, text="🚀 Start 1v1 Pairing", font=ctk.CTkFont(size=16), command=self.backend.start_pairing, state="disabled"); self.pairing_button.grid(row=0, column=0, padx=5, pady=5, sticky="ew") #
        self.discovery_button = ctk.CTkButton(control_frame, text="➕ Allow New Devices to Join", font=ctk.CTkFont(size=16), command=self.backend.toggle_discovery_mode, state="disabled"); self.discovery_button.grid(row=0, column=1, padx=5, pady=5, sticky="ew") #
        self.stop_button = ctk.CTkButton(control_frame, text="🛑 STOP", font=ctk.CTkFont(size=16, weight="bold"), command=self.backend.send_stop_command, state="disabled", fg_color="#D32F2F", hover_color="#B71C1C"); self.stop_button.grid(row=0, column=2, padx=(5,10), pady=5, sticky="e") #
        # Scenario control buttons
        self.scenario1_button = ctk.CTkButton(control_frame, text="🎯 Scenario 1 Start", font=ctk.CTkFont(size=16), command=self.backend.send_scenario1_start, state="disabled"); self.scenario1_button.grid(row=1, column=0, padx=5, pady=5, sticky="ew") #
        self.scenario2_button = ctk.CTkButton(control_frame, text="🧩 Scenario 2 Start", font=ctk.CTkFont(size=16), command=self.backend.send_scenario2_start, state="disabled"); self.scenario2_button.grid(row=1, column=1, padx=5, pady=5, sticky="ew") #
        
        # --- Tab View (Simplified) ---
        self.tab_view = ctk.CTkTabview(self, text_color="white"); self.tab_view.grid(row=2, column=0, padx=10, pady=10, sticky="nsew") #
        self.tab_view.add("Profile Management"); self.tab_view.add("Raw Log") #
        
        # --- Profile Management Tab ---
        profile_tab_frame = self.tab_view.tab("Profile Management"); profile_tab_frame.grid_columnconfigure(1, weight=3); profile_tab_frame.grid_columnconfigure(0, weight=1); profile_tab_frame.grid_rowconfigure(0, weight=1) #
        self.device_list_frame = ctk.CTkScrollableFrame(profile_tab_frame, label_text="Connected Devices"); self.device_list_frame.grid(row=0, column=0, sticky="nsew", padx=10, pady=10) #
        self.profile_frame = ctk.CTkFrame(profile_tab_frame); self.profile_frame.grid(row=0, column=1, sticky="nsew", padx=10, pady=10); self.profile_frame.grid_columnconfigure(1, weight=1) #
        profile_fields = ["Name", "Department", "Hobbies", "Birthday"] #
        for i, field in enumerate(profile_fields):
            label = ctk.CTkLabel(self.profile_frame, text=f"{field}:", anchor="w"); label.grid(row=i, column=0, padx=10, pady=10, sticky="w") #
            entry = ctk.CTkEntry(self.profile_frame, width=300, state="disabled"); entry.grid(row=i, column=1, padx=10, pady=10, sticky="ew"); self.profile_entries[field.lower()] = entry #
        button_frame = ctk.CTkFrame(self.profile_frame, fg_color="transparent"); button_frame.grid(row=len(profile_fields), column=0, columnspan=2, padx=10, pady=20, sticky="ew"); button_frame.grid_columnconfigure(0, weight=1); button_frame.grid_columnconfigure(1, weight=1) #
        self.save_profile_button = ctk.CTkButton(button_frame, text="Save Current Profile", command=self.handle_save_profile_button, state="disabled"); self.save_profile_button.grid(row=0, column=0, padx=5, sticky="ew") #
        self.remove_profile_button = ctk.CTkButton(button_frame, text="Remove This User", command=lambda: self.backend.remove_slave(self.currently_selected_id), state="disabled", fg_color="#D32F2F", hover_color="#B71C1C"); self.remove_profile_button.grid(row=0, column=1, padx=5, sticky="ew") #

        # --- Raw Log Tab ---
        self.raw_log_textbox = ctk.CTkTextbox(self.tab_view.tab("Raw Log"), state="disabled"); self.raw_log_textbox.pack(fill="both", expand=True, padx=5, pady=5) #

    # --- Event Handlers and Callbacks ---

    def handle_connect_button(self):
        if self.backend.is_connected: self.backend.disconnect() #
        else: self.backend.connect(self.port_menu.get()) #

    def on_closing(self):
        if self.backend.is_connected: self.backend.disconnect() #
        self.destroy() #

    def scan_ports(self):
        ports = self.backend.get_ports() #
        self.port_menu.configure(values=ports if ports else ["No ports found"]) #
        if ports: self.port_menu.set(ports[0]) #
    
    def handle_save_profile_button(self):
        if self.currently_selected_id is None: return #
        device_id_str = str(self.currently_selected_id) #
        with self.backend.data_lock: #
            if device_id_str not in self.backend.config["user_profiles"]: #
                self.backend.config["user_profiles"][device_id_str] = {}
            for field, entry_widget in self.profile_entries.items(): #
                self.backend.config["user_profiles"][device_id_str][field] = entry_widget.get()
        if self.backend.save_config(): #
            self.save_profile_button.configure(text="Profile Saved!", fg_color="green")
            self.after(2000, lambda: self.save_profile_button.configure(text="Save Current Profile", fg_color=ctk.ThemeManager.theme["CTkButton"]["fg_color"])) #
            # Update the button text in the device list
            with self.backend.data_lock: #
                new_name = self.backend.config["user_profiles"][device_id_str].get("name", "")
                button_text = f"ID: {device_id_str} - {new_name}" if new_name else f"Device #{device_id_str}"
                self.device_buttons[self.currently_selected_id].configure(text=button_text)
                # Send NAME_ command to master (triggers BLINK_WHITE on target)
                if new_name:
                    self.backend.send_name(self.currently_selected_id, new_name)

    def on_status_update(self, message, color):
        self.status_label.configure(text=message, text_color=color) #

    def on_connection_status_update(self, is_connected, message):
        self.connect_button.configure(text="Disconnect" if is_connected else "Connect") #
        self.status_label.configure(text=message, text_color="green" if is_connected else "gray") #
        for btn in [self.stop_button, self.discovery_button, self.pairing_button, self.scenario1_button, self.scenario2_button]: #
            btn.configure(state="normal" if is_connected else "disabled") #
        if not is_connected:
            self.on_slave_update({}, {})

    def log_to_textbox(self, textbox, message):
        textbox.configure(state="normal"); textbox.insert(tk.END, message + "\n"); textbox.see(tk.END); textbox.configure(state="disabled") #

    def on_raw_log(self, message):
        self.log_to_textbox(self.raw_log_textbox, message) #

    def display_profile(self, device_id):
        self.currently_selected_id = device_id #
        with self.backend.data_lock: #
            profile = self.backend.config.get("user_profiles", {}).get(str(device_id), {}) #
        for field, entry_widget in self.profile_entries.items(): #
            entry_widget.configure(state="normal"); entry_widget.delete(0, tk.END); entry_widget.insert(0, profile.get(field, "")) #
        self.save_profile_button.configure(state="normal"); self.remove_profile_button.configure(state="normal") #

    def on_slave_update(self, slaves, profiles):
        for widget in self.device_list_frame.winfo_children(): #
            widget.destroy()
        
        self.device_buttons.clear() #

        if not slaves:
            ctk.CTkLabel(self.device_list_frame, text="No devices discovered yet...").pack(pady=10) #
        else:
            for device_id in sorted(slaves.keys()): #
                profile = profiles.get(str(device_id), {}) #
                name = profile.get("name", "") #
                button_text = f"ID: {device_id} - {name}" if name else f"Device #{device_id}"
                btn = ctk.CTkButton(self.device_list_frame, text=button_text, command=lambda d_id=device_id: self.display_profile(d_id)); btn.pack(fill="x", pady=2, padx=5); self.device_buttons[device_id] = btn #
        
        if self.currently_selected_id and self.currently_selected_id not in slaves: #
            for entry in self.profile_entries.values(): entry.delete(0, tk.END); entry.configure(state="disabled") #
            self.save_profile_button.configure(state="disabled"); self.remove_profile_button.configure(state="disabled"); self.currently_selected_id = None #
        
        self.on_mode_update()

    def on_mode_update(self, is_discovery_mode=None):
        with self.backend.data_lock: #
            if is_discovery_mode is None:
                 is_discovery_mode = self.backend.is_discovery_mode #
            num_devices = len(self.backend.discovered_slaves) #

        can_run_game = not is_discovery_mode and num_devices >= 2 and num_devices % 2 == 0 #
        self.pairing_button.configure(state="normal" if can_run_game else "disabled", text=f"🚀 Start 1v1 Pairing ({num_devices} Devices)") #
        self.scenario1_button.configure(state="normal" if can_run_game else "disabled") #
        self.scenario2_button.configure(state="normal" if can_run_game else "disabled") #
        
        if is_discovery_mode:
            self.discovery_button.configure(text="✅ Finalize & Stop Joining", fg_color="#1F6AA5") #
        else:
            self.discovery_button.configure(text="➕ Allow New Devices to Join", fg_color=ctk.ThemeManager.theme["CTkButton"]["fg_color"]) #

if __name__ == "__main__":
    app = SocialTagControlUI()
    app.mainloop()