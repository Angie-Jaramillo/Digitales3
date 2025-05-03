import machine
import utime
import sys

global paso
paso = 0

r_pwm = machine.PWM(machine.Pin(16), 20000, duty_u16=0)
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
    print('Tiempo(ms), Duty(%), RPM')
    
    send_data = utime.ticks_ms()
    timer_start = utime.ticks_ms()
    Move(reftoPWM(ref))

    while end:
        timer_elapsed = utime.ticks_diff(utime.ticks_ms(), timer_start)
        
        if timer_elapsed >= 500:
            rpm = paso*60/20
            print('{}, {}, {}'.format(utime.ticks_diff(utime.ticks_ms(), send_data),ref,rpm))
            paso = 0
            timer_start = utime.ticks_ms()       
    return

def main():
    encoder.irq(trigger = machine.Pin.IRQ_RISING, handler=encoder_handler)
    Start(20)
            
main()
                                                                                                                