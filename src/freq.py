import hs1101
import math

R4= 49.9e3
R2= 576e3

F555 = (R4+2*R2)*0.693e-12

def freq555(rh):
    c = hs1101.func(rh,0)
    return 1/(c*F555)

#================================================    
if __name__ == "__main__":
    for rh in range(0,105,5):
        print("rh:", rh, "  freq=", freq555(rh))