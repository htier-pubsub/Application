import struct
from pyModbusTCP.client import ModbusClient
import config_parameters
import time


def read_holding_registers():
    # Create a Modbus client instance
    client = ModbusClient(host=config_parameters.HOST_IP, port=config_parameters.PORT, auto_open=True)

    try:
        # Read holding registers
        regs_l = client.read_holding_registers(config_parameters.REG_ADDR, config_parameters.REG_NB)

        if regs_l:
            regs_float = []
            for i in range(0, len(regs_l), 2):
                mypack = struct.pack('>HH', regs_l[i + 1], regs_l[i])
                f = struct.unpack('>f', mypack)
                regs_float.append(f[0])

            print("Read values from holding registers:", regs_float)
        else:
            print("Read error: No data received from Modbus server.")

    except Exception as e:
        print(f"Error reading holding registers: {e}")


if __name__ == "__main__":
    while True:
        read_holding_registers()
        time.sleep(5)  # Adjust the sleep time as needed
