#!/usr/bin/env python3
"""
HID debugging script to monitor USB HID device reports
"""

import sys
import time

try:
    import hid
except ImportError:
    print("Installing pyhidapi...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyhidapi", "--break-system-packages"])
    import hid

def find_rp2040_hid():
    """Find RP2040 HID interfaces"""
    devices = hid.enumerate()
    rp2040_devices = []
    all_devices = []
    
    print("All HID devices:")
    for device in devices:
        all_devices.append(device)
        print(f"  VID:PID: {device['vendor_id']:04x}:{device['product_id']:04x}")
        print(f"    Product: {device.get('product_string', 'Unknown')}")
        print(f"    Manufacturer: {device.get('manufacturer_string', 'Unknown')}")
        print(f"    Usage Page: {device.get('usage_page', 'N/A')}")
        print(f"    Usage: {device.get('usage', 'N/A')}")
        print(f"    Path: {device['path']}")
        print()
    
    for device in devices:
        if device['vendor_id'] == 0x2e8a:  # Raspberry Pi Foundation
            rp2040_devices.append(device)
            print(f"Found RP2040 HID device:")
            print(f"  Product: {device.get('product_string', 'Unknown')}")
            print(f"  Manufacturer: {device.get('manufacturer_string', 'Unknown')}")
            print(f"  VID:PID: {device['vendor_id']:04x}:{device['product_id']:04x}")
            print(f"  Usage Page: {device.get('usage_page', 'N/A')}")
            print(f"  Usage: {device.get('usage', 'N/A')}")
            print(f"  Interface: {device.get('interface_number', 'N/A')}")
            print(f"  Path: {device['path']}")
            print()
    
    return rp2040_devices, all_devices

def monitor_hid_device(device_path):
    """Monitor HID reports from device"""
    try:
        device = hid.device()
        device.open_path(device_path)
        
        print(f"Opened HID device: {device_path}")
        print(f"Manufacturer: {device.get_manufacturer_string()}")
        print(f"Product: {device.get_product_string()}")
        print(f"Serial: {device.get_serial_number_string()}")
        print("\nWaiting for HID reports... (Press Ctrl+C to stop)")
        
        device.set_nonblocking(True)
        
        while True:
            data = device.read(64, timeout_ms=100)
            if data:
                hex_data = ' '.join(f'{b:02x}' for b in data)
                print(f"HID Report: {hex_data}")
                
                # Decode as gamepad report if it looks right
                if len(data) >= 9 and data[0] == 0x01:
                    buttons = data[1] | (data[2] << 8)
                    axes = data[3:9]
                    print(f"  Buttons: 0x{buttons:04x}")
                    print(f"  Axes: {axes}")
            
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print("\nStopping...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        try:
            device.close()
        except:
            pass

def main():
    print("=== RP2040 HID Device Debugger ===\n")
    
    devices, all_devices = find_rp2040_hid()
    
    if not devices:
        print("No RP2040 HID devices found!")
        if not all_devices:
            print("No HID devices found at all!")
        return
    
    # Look for HID gamepad devices (Usage Page 1, Usage 5)
    gamepad_devices = [d for d in devices if d.get('usage_page') == 1 and d.get('usage') == 5]
    
    if gamepad_devices:
        print("Found gamepad HID devices:")
        for device in gamepad_devices:
            print(f"Monitoring: {device['path']}")
            monitor_hid_device(device['path'])
            break
    else:
        print("No HID gamepad devices found. Trying first HID device...")
        hid_devices = [d for d in devices if d.get('usage_page') is not None]
        if hid_devices:
            monitor_hid_device(hid_devices[0]['path'])
        else:
            print("No HID interfaces found on RP2040 device!")

if __name__ == "__main__":
    main()