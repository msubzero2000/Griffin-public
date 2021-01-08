from socket_sender.socket_sender import  SocketSender
import time

socket_sender = SocketSender()
ctr = 0
roll = 0
pitch = 0
gameState = 0
leftWingAngle = 0
rightWingAngle = 0
bodyHeight = 0

while True:
    if ctr < 150:
        socket_sender.send(0,  0,  gameState,  leftWingAngle,  rightWingAngle,  bodyHeight)
        
        if int(ctr / 15) % 2 == 0:
            leftWingAngle = 40
        else:
            leftWingAngle = -40

        if int(ctr / 15) % 2 == 0:
            rightWingAngle = -35
            bodyHeight = 5
        else:
            rightWingAngle = 35
            bodyHeight = 0

    elif ctr < 200:
        gameState = 1
        socket_sender.send(roll,  pitch,  gameState,  0,  0,  0)
        
        if int(ctr / 60) % 2 == 0:
            roll = -30
        else:
            roll = 30
            
        pitch += -1
    else:
        ctr = 0
        roll = 0
        pitch = 0
        gameState = 0
        leftWingAngle = 0
        rightWingAngle = 0
        
    ctr += 1
    
    time.sleep(1/10)
    print(f"Socket roll={roll} pitch={pitch} SENT!!")
