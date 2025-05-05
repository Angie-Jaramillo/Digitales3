#Se importan las librerias
import machine
import utime
import sys

#Declaracion de variables globales
global paso
paso = 0

#Inicializacion de perifericos
r_pwm = machine.PWM(machine.Pin(16), 20000, duty_u16=0)
l_pwm = machine.PWM(machine.Pin(17), 20000, duty_u16=0)

enable = machine.Pin(18, machine.Pin.OUT)
enable.on()

encoder = machine.Pin(22, machine.Pin.IN)

#Funciones

#Funcion conectada  ala interrupcion del encoder
#esta va a ir sumando a paso cada que detecta un
#flanco de subida
def encoder_handler(pin):
    global paso
    paso += 1

#Funcion para la conversion de ancho de pulso a
#lo que recibe los motores
def reftoPWM(ref):
    x = (ref*65535)/100
    return x

#Funcion para enviar la señal a los motores
#esta cuenta con el posible cambio de direccion
def Move(k):
    if k>65535:
        l_pwm.duty_u16(0)
        r_pwm.duty_u16(65535)
    elif k<-65535:
        r_pwm.duty_u16(0)
        l_pwm.duty_u16(65535)
    elif (k>=0) and (k<65535):
        l_pwm.duty_u16(0)
        r_pwm.duty_u16(int(k))
    elif (k>-65535) and (k<0):
        r_pwm.duty_u16(0)
        l_pwm.duty_u16(int(k))

#Funcion que mueve el motor y cuenta los RPM's
#esta recibe un ancho de pulso, y se gun este valor es la magnitud de cambio
#del ancho de pulso
def Start(value):
    global paso
    ref  = 0
    rpm = 0
    buff = ['Tiempo(ms), Duty(%), RPM']
    
    send_data = utime.ticks_ms()
    chang_ref = utime.ticks_ms()
    timer_start = utime.ticks_ms()
    Move(reftoPWM(ref))

    while ref <= 100:
        timer_elapsed = utime.ticks_diff(utime.ticks_ms(), timer_start)
        
        if timer_elapsed >= 10:
            rpm = paso*60/20
            buff.append('{}, {}, {}'.format(utime.ticks_diff(utime.ticks_ms(), send_data),ref,rpm))
            paso = 0
            timer_start = utime.ticks_ms()       
        
        if utime.ticks_diff(utime.ticks_ms(), chang_ref) >= 2000:
            ref = ref + value
            if ref > 100:
                Move(reftoPWM(100))
            else:
                Move(reftoPWM(ref))     
            chang_ref = utime.ticks_ms()
            
    print(buff)
    buff.clear()
    buff = ['Tiempo(ms), Duty(%), RPM']
    Move(0)
    return

#Funcion que mueve el motor a duty constante durante 5s
def PWM(value):
    global paso
    rpm = 0
    print('Duty(%), RPM')
    
    send_data = utime.ticks_ms()
    chang_ref = utime.ticks_ms()
    timer_start = utime.ticks_ms()
    Move(reftoPWM(value))
    
    while utime.ticks_diff(utime.ticks_ms(), chang_ref) <= 5000:
        timer_elapsed = utime.ticks_diff(utime.ticks_ms(), timer_start)
        
        if timer_elapsed >= 100:
            rpm = paso*60/20
            paso = 0
            timer_start = utime.ticks_ms()
            
        if utime.ticks_diff(utime.ticks_ms(), send_data) >= 500:
            print('{}    ,  {}'.format(value, rpm))
            send_data = utime.ticks_ms()
    
    Move(0)
    return

#Main del codigo, se le pide al usuario que ingrese el comando
def main():
    encoder.irq(trigger = machine.Pin.IRQ_RISING, handler=encoder_handler)
    while True:
        print("Ingerse un comando:")
        Comando = sys.stdin.readline()
        if Comando[0:5] == "START":
            if int(Comando[6:-2]) > 0 and int(Comando[6:-2]) <= 100:
                Start(int(Comando[6:-2]))
            else:
                print("Valor incorrecto.")
        elif Comando[0:3] == "PWM":
            if int(Comando[4:-2]) > 0 and int(Comando[4:-2]) <= 100:
                PWM(int(Comando[4:-2]))
            else:
                print("Valor incorrecto.")
        else:
            print("Comando incorrecto.")
        Comando = '\0'
            
main()
                                                                                                                