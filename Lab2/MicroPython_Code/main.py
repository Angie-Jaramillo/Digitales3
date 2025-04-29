import machine
import utime
import sys

global paso
paso = 0

r_pwm = machine.PWM(machine.Pin(16), 20000, duty_u16=65535)
l_pwm = machine.PWM(machine.Pin(17), 20000, duty_u16=0)

enable = machine.Pin(18, machine.Pin.OUT)
enable.on()

encoder = machine.Pin(22, machine.Pin.IN)

def encoder_handler(pin):
    global paso
    paso += 1
    
def reftoPWM(ref):
    x = (ref*65535)/100
    return x
    
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
        
def Start(value):
    global paso
    ref  = 0
    rpm = 0
    buff = ['Tiempo(ms), Duty(%), RPM']
    div_tiempo = 100
    
    send_data = utime.ticks_ms()
    chang_ref = utime.ticks_ms()
    timer_start = utime.ticks_ms()
    Move(reftoPWM(ref))
    
    if(value >= 20):
        div_tiempo = 10

    while ref <= 100:
        timer_elapsed = utime.ticks_diff(utime.ticks_ms(), timer_start)
        
        if timer_elapsed >= div_tiempo:
            rpm = paso*60/20
            paso = 0
            timer_start = utime.ticks_ms()       
            buff.append('{}, {}, {}'.format(utime.ticks_diff(utime.ticks_ms(), send_data),ref,rpm))
        
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
    return

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
    
    return

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
                                                                                                                