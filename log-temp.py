import serial, time, datetime

ser = serial.Serial('COM3', 9600)
time.sleep(2)

ser.write(b's')
temp = ser.readline()
st = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

output = st + ', ' + str(temp,'ascii').rstrip() + '\n'

print(output)

logFile = open('log.txt', 'a')
logFile.write(output)
logFile.close()
