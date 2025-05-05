import machine
import utime
import sys

global paso
paso = 0

r_pwm = machine.PWM(machine.Pin(16), 20000, duty_u16=0)
l_pwm = machine.PWM(machine.Pin(17), 20000, duty_u16=0)

enable = machine.Pin(18, machine.Pin.OUT)
enable.on()

encoder = machine.Pin(19, machine.Pin.IN)

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
    direc = 1
    end = True
    buff = []
    
    send_data = utime.ticks_ms()
    chang_ref = utime.ticks_ms()
    timer_start = utime.ticks_ms()
    Move(reftoPWM(ref))

    while end:
        timer_elapsed = utime.ticks_diff(utime.ticks_ms(), timer_start)
        
        if timer_elapsed >= 4:
            rpm = paso*60/20
            buff.append('{} {} {}\n'.format(utime.ticks_diff(utime.ticks_ms(), send_data),ref,rpm))
            paso = 0
            timer_start = utime.ticks_ms()       
        
        if utime.ticks_diff(utime.ticks_ms(), chang_ref) >= 3000:
            if ref >= 100:
                direc = 0
            if direc == 0 and ref == 0:
                end = False
            if direc == 1:
                ref = ref + value
            elif direc == 0:
                ref = ref - value
            Move(reftoPWM(ref))
            chang_ref = utime.ticks_ms()
            
    print(buff)
    buff.clear()
    buff = ['Tiempo(ms), Duty(%), RPM']
    Move(0)
    return

def main():
    encoder.irq(trigger = machine.Pin.IRQ_RISING, handler=encoder_handler)
    Start(20)
            
main()
                                                                                                                