from pymodbus.client import ModbusSerialClient
from pymodbus.transaction import ModbusRtuFramer
import time
import logging

FORMAT = ('%(message)-15s')
logging.basicConfig(format=FORMAT)
log = logging.getLogger()
log.setLevel(logging.DEBUG)

master = ModbusSerialClient(framer=ModbusRtuFramer, port = '/dev/ttyACM0', stopbits=1, bytesize=8, parity='N', baudrate=9600)

connexion = master.connect()
print(master)
print("\033[92m-------------------client connected----------------------\033[0m")


while True:

    print("before write")
    # sending = master.write_registers(5, "bonjour".encode(), 0x01)
    # time.sleep(1)
    # print("after write") 
    receive = master.read_holding_registers(5, 7, 0x01)
    # print(receive)
    # time.sleep(1)
    print("read")