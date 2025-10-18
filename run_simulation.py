import sys
import tkinter as tk
import tkinter.font
import time, random, os
import threading

'''
argument 1    : map filename
argument 2    : max number of turns
argument 3    : stalling time between each turn (seconds)
arguments 4~N : players (au nombre de N-3)
'''

# music
def playsound():
    #os.system("mpv audio.MP3 &")
    print("Done!\n")

# global constants
try:
    MAPNAME   = sys.argv[1]
    MAX_TURNS = int(sys.argv[2])
    DT = float(sys.argv[3])
    N_PLAYERS = len(sys.argv)-4
    MAPLEN    = 0
    assert(N_PLAYERS>0)
except:
    print("Usage : python3 run___.py <mapName> <maxTurns> <initial_DT> <players>",file=sys.stderr)
    assert 0
map = []
''' map = (string * int * int | int array) array array --> (tileName, resource, tileMeta) '''

root = tk.Tk()

#graphics constant
''' Change your monitor resolution here '''
WIDTH=root.winfo_screenwidth()
HEIGHT=root.winfo_screenheight()
''' Change your monitor resolution here '''
ADDTILE_R=5
ADDTILE_B=4
PLAYER_COLOR = ["#dd0000", "#dddd00", "#00dd00", "#6666ff"]
PLAYER_COLOR_2 = ["#dd8888", "#dddd88", "#88dd88", "#aaaaff"]
HALT=False

# player-related constants
PLAYER_RSCS = [20 for i in range(N_PLAYERS)]
PLAYER_CARRY = [0 for i in range(N_PLAYERS)]
PLAYER_SCORE = [0 for i in range(N_PLAYERS)]
PLAYER_SPAWN = []
PLAYER_MINIONS=[{} for _ in range(N_PLAYERS)]
''' ((x,y > cap,hp,maxCap,atk) dict) array '''
PLAYER_NAMES = []
for p in range(N_PLAYERS):
    pName = sys.argv[4+p]
    if(pName[0] != '.'):
        pName = "python3 "+pName
    PLAYER_NAMES.append(pName)

random.shuffle(PLAYER_NAMES)     # random starting points

# log file (useful for debugging)
logFile=open("logFile.txt","w")

# check if the arguments (ie the player functions) are valid
for pname in PLAYER_NAMES:
    assert(
        len(pname) >= 3 and (
            pname[0] == '.' and pname[1] == "/"     # executable file
        ) or (
            pname[-1] == 'y' and pname[-2] == 'p' and pname[-3] == '.'      # python file
        )
    )

# pre-generate all random events (declaring it here because needed by the writer)
randomEventList=[]
randomEventFullList=[]

## data regarding random events
currentRandomEvent=""
addHP=0
addDMG=0
addCapa=0
addPump=0
freePump=0

# writes data
def write_player_data(pl_i,curTurn):
    f=open("mapData.txt", "w")

    # map
    print(MAPLEN,file=f)
    for line in range(MAPLEN):
        for col in range(MAPLEN):
            (spec,rsc,meta)=map[line][col]
            if(spec=="RESO"):
                print("R,",file=f,end="")
            elif(spec=="WALL"):
                print("W,",file=f,end="")
            elif(spec=="DASH"):
                print("D,",file=f,end="")
            elif(spec=="PROT"):
                print("S,",file=f,end="")
            elif(spec=="BONK"):
                print("F,",file=f,end="")
            elif(spec=="GOLD"):
                print("M,",file=f,end="")
            elif(spec=="SPED"):
                print("P,",file=f,end="")
            else:
                print(f"ERROR : unrecognized tile type ({spec})",file=sys.stderr)
                assert 0

            print(f"{rsc},{meta}",file=f,end=(" " if col != MAPLEN-1 else ""))
        print("",file=f)

    # players
    nMinions=0
    for pl in range(N_PLAYERS):
        playerMinions=PLAYER_MINIONS[pl]
        nMinions += len(playerMinions)

    print(nMinions,file=f)

    for pl in range(N_PLAYERS):
        for (mX,mY),(mCar,mHp,mSize,mAtk) in PLAYER_MINIONS[pl].items():
            print(f"{pl},{mX},{mY},{mCar},{mHp},{mSize},{mAtk}",file=f)

    # current player + your resources
    print(pl_i, PLAYER_RSCS[pl_i],file=f)
    print(PLAYER_SPAWN[pl_i][0],PLAYER_SPAWN[pl_i][1],file=f)

    # turn data
    print(f"{curTurn} {MAX_TURNS}",file=f)

    # random events
    for i in range(len(randomEventFullList)):
        print(f"{randomEventFullList[i]},{(i+1)*150}",file=f,end=" ")

    print("",file=f)

    f.close()

# checks if the minion is attacking (can be optimized, but future me will have to deal with this)
def targetMinionData(pl_i,xdest,ydest):
    for p in range(N_PLAYERS):
        if(p != pl_i):
            if((xdest,ydest) in PLAYER_MINIONS[p].keys()):
                return p
    return -1

# function to avoid falling off the map
def areValid(i,j):
    return 0 <= i < MAPLEN and 0 <= j < MAPLEN

def applyBonus(pl_i,x0,y0,xd,yd,mved):
    minData=PLAYER_MINIONS[pl_i][(xd,yd)]
    tile=map[xd][yd]
    if(tile[0]=="DASH"):
        # repeat movement
        print(f">>> DASH {tile[2]}",file=logFile)
        print(f"[{pl_i}] ",file=logFile,end="")
        Dx=xd-x0
        Dy=yd-y0
        curX=xd
        curY=yd
        for _ in range(tile[2]):
            inBounds=areValid(curX+Dx,curY+Dy)
            target=targetMinionData(pl_i,curX+Dx,curY+Dy)
            if((curX+Dx,curY+Dy) in PLAYER_MINIONS[pl_i].keys()):
                # dont forget to put this piece of sh*t before the second condition, otherwise minions will merge and break everything >:/
                # transfer
                minTarget=PLAYER_MINIONS[pl_i][(curX+Dx,curY+Dy)]
                toTransfer=min(minData[0],(1+addCapa)*minTarget[2]-minTarget[0])
                minData[0] -= toTransfer
                minTarget[0] += toTransfer
            elif(target == -1):
                if(inBounds):
                    tile=map[curX+Dx][curY+Dy]
                    if(tile[0] != "WALL"):
                        # move
                        curX += Dx
                        curY += Dy
            else:
                # attack (ONE-SIDED)
                minHit=PLAYER_MINIONS[target][(curX+Dx,curY+Dy)]
                minHit[1] -= minData[3]*(1+addDMG)//(1+addHP)
        minCpy = [minData[0],minData[1],minData[2],minData[3]]
        del PLAYER_MINIONS[pl_i][(xd,yd)]
        PLAYER_MINIONS[pl_i][(curX,curY)] = minCpy

        if((areValid(curX,curY)) and (curX != xd or curY != yd)):
            applyBonus(pl_i,curX-Dx,curY-Dy,curX,curY,mved)
        else:
            mved[(curX,curY)]=True


    elif(tile[0]=="PROT"):
        # more HP
        print(f">>> PROT {tile[2]}",file=logFile)
        print(f"[{pl_i}] ",file=logFile,end="")
        minData[1]+=tile[2]
        map[xd][yd]=("RESO",tile[1],0)

    elif(tile[0]=="BONK"):
        # more damage
        print(f">>> BONK {tile[2]}",file=logFile)
        print(f"[{pl_i}] ",file=logFile,end="")
        minData[3]+=tile[2]
        map[xd][yd]=("RESO",tile[1],0)

    elif(tile[0]=="GOLD"):
        print(f">>> GOLD {tile[2]}",file=logFile)
        print(f"[{pl_i}] ",file=logFile,end="")
        # replenish nearby tiles
        for dx in range(-2,3):
            for dy in range(-2,3):
                nx=xd+dx
                ny=yd+dy
                if(areValid(nx,ny)):
                    tl = map[nx][ny]
                    map[nx][ny] = (tl[0],min(10,tl[1]+tile[2]),tl[2])
        map[xd][yd]=("RESO",tile[1],0)

    elif(tile[0]=="SPED"):
        # todo #
        pass

def read_player_data(pl_i):
    minionMoved={}      # to avoid moving the same minion mutiple times
    with open("answer.txt", "r") as file:
        for line in file:
            print(f"{[pl_i]}",file=logFile,end=" ")
            # create new minion
            if(line[0] == "C"):
                dat=line.split(" ")
                _,hp,size,atk=dat
                hp,size,atk=int(hp),int(size),int(atk)
                if(hp>0 and size>=0 and atk >=0 and 2*hp+2*size+atk <= 25):
                    if(not (PLAYER_SPAWN[pl_i] in PLAYER_MINIONS[pl_i].keys())):   # check if spawn is free
                        if(PLAYER_RSCS[pl_i] >= hp + size + atk):       # check is the player has enough resources
                            PLAYER_MINIONS[pl_i][PLAYER_SPAWN[pl_i]] = [0,2*hp,2*size,atk]
                            PLAYER_RSCS[pl_i] -= hp + size + atk
                            minionMoved[PLAYER_SPAWN[pl_i]]=True
                            print(f"NEW_MINION {hp},{size},{atk}",file=logFile,end="\n")
                        else:
                            print(f"NEW_MINION_BROKE {hp},{size},{atk}",file=logFile,end="\n")
                    else:
                        print(f"NEW_MINION_FAIL {hp},{size},{atk}",file=logFile,end="\n")
                else:
                    print(f"NEW_MINION_INVALID {hp},{size},{atk}",file=logFile,end="\n")

            # move existing minion
            else:
                dat=line.split(" ")
                x,y,xdest,ydest=dat
                x,y,xdest,ydest=int(x),int(y),int(xdest),int(ydest)
                # have to do this because for some reason list(map(int,line.split(" "))) throws an error 'list is not callable
                
                # check if the x,y coords is a minion
                if((not ((x,y) in minionMoved.keys())) and (x,y) in PLAYER_MINIONS[pl_i].keys()):
                    minData=PLAYER_MINIONS[pl_i][(x,y)]
                    if(xdest==x and ydest==y):
                        # pump
                        tile=map[x][y]
                        for _ in range(1+addPump):
                            if(tile[1] > 0 and minData[0] < minData[2]*(1+addCapa)):
                                    map[x][y] = (tile[0],tile[1]-(1-freePump),tile[2])
                                    minData[0]+=1
                                    PLAYER_CARRY[pl_i]+=1
                                    minionMoved[(x,y)]=True
                                    print(f"PUMP {x},{y}",file=logFile,end="\n")
                            else:
                                print(f"PUMP_EMPTY {x},{y}",file=logFile,end="\n")

                    elif(abs(xdest-x)+abs(ydest-y)==1 and areValid(xdest,ydest)):
                        # movement-based action
                        # move into the depot
                        if((xdest,ydest) == PLAYER_SPAWN[pl_i]):
                            print(f"DEPOSIT {x},{y},{minData[0]}",file=logFile,end="\n")
                            PLAYER_SCORE[pl_i] += minData[0]
                            PLAYER_RSCS[pl_i] += minData[0]
                            PLAYER_CARRY[pl_i]-=minData[0]
                            minionMoved[(x,y)]=True
                            minData[0] = 0

                        # moving into a friendly minion
                        elif((xdest,ydest) in PLAYER_MINIONS[pl_i].keys()):
                            minTarget=PLAYER_MINIONS[pl_i][(xdest,ydest)]
                            toTransfer=min(minData[0],(1+addCapa)*minTarget[2]-minTarget[0])
                            #print(minData[0],minTarget[2]-minTarget[0])
                            minData[0] -= toTransfer
                            minTarget[0] += toTransfer
                            minionMoved[(x,y)]=True
                            print(f"TRANSFER {x},{y},{xdest},{ydest},{toTransfer}",file=logFile,end="\n")

                        else:
                            # empty space
                            target=targetMinionData(pl_i,xdest,ydest)
                            if(target == -1):
                                if(map[xdest][ydest][0] != "WALL" and not ((xdest,ydest) in PLAYER_SPAWN)): # not moving into a wall
                                    minCpy = [minData[0],minData[1],minData[2],minData[3]]
                                    del PLAYER_MINIONS[pl_i][(x,y)]
                                    PLAYER_MINIONS[pl_i][(xdest,ydest)] = minCpy
                                    minionMoved[(xdest,ydest)]=True
                                    applyBonus(pl_i,x,y,xdest,ydest,minionMoved)
                                    print(f"MOVE {x},{y},{xdest},{ydest}",file=logFile,end="\n")
                                else:
                                    print(f"BONKED {x},{y},{xdest},{ydest}",file=logFile,end="\n")

                            # attack
                            else:
                                minHit=PLAYER_MINIONS[target][(xdest,ydest)]
                                minHit[1] -= minData[3]*(1+addDMG)//(1+addHP)
                                minData[1] -= minHit[3]*(1+addDMG)//(1+addHP)
                                if(minHit[1] > 0):
                                    # if it didnt kill, it counts as a move
                                    minionMoved[(x,y)]=True
                                elif(minData[1] > 0):
                                    # if kill + survive, move
                                    minCpy = [minData[0],minData[1],minData[2],minData[3]]
                                    del PLAYER_MINIONS[pl_i][(x,y)]
                                    PLAYER_MINIONS[pl_i][(xdest,ydest)] = minCpy
                                    minionMoved[(xdest,ydest)]=True

                                print(f"ATTACK {x},{y},{xdest},{ydest},{minData[3]}",file=logFile,end="\n")

                    else:
                        # invalid
                        print(f"INVALID_MOVE {x},{y},{xdest},{ydest}",file=logFile,end="\n")
                
                else:
                    # invalid
                    print(f"INVALID_MINION {x},{y},{xdest},{ydest}",file=logFile,end="\n")

# plays the turn of a player
def execute_player(pl_i,curTurn):
    if(pl_i < 0):
        print(f"ERROR : trying to play negative player {pl_i}",file=sys.stderr)

    if(pl_i >= N_PLAYERS):
        print(f"ERROR : trying to play overflowing player {pl_i}",file=sys.stderr)
    
    pname = PLAYER_NAMES[pl_i]
    try:
        write_player_data(pl_i,curTurn)
    except:
        print("Error while writing.",file=sys.stderr)

    execGood=False
    try:
        if os.name == "posix":
            os.system(pname)
        elif os.name =="nt":
            os.system('python '+programme[2:])
        execGood=True
    except:
        print(f"Error while executing player {pl_i}'s code ({PLAYER_NAMES[pl_i]}).",file=sys.stderr)

    if(execGood):
        #read_player_data(pl_i)
        try:
            read_player_data(pl_i)
        except:
            print(f"Error while reading data for player {pl_i}.",file=sys.stderr)

# remove all players with HP<=0
def killDeadMinions():
    for p in range(len(PLAYER_MINIONS)):
        minionList=PLAYER_MINIONS[p]
        toDel=[]
        for (x,y),(cap,hp,_,_) in minionList.items():
            if(hp <= 0):
                toAddTile=map[x][y]
                map[x][y] = (toAddTile[0],min(10,toAddTile[1]+cap),toAddTile[2])
                PLAYER_CARRY[p]-=cap
                toDel.append((x,y))
        for (x,y) in toDel:
            del minionList[(x,y)]

# .isdigit() but for relative numbers as well
def isRelative(str):
    return (str.isdigit() or (len(str)>0 and str[0]=="-" and str[1:].isdigit()))

# useful parsing functions
def toTileName(str):
    if(isRelative(str)):
        if(int(str)>=0):
            return "RESO"
        else:
            return "WALL"
    elif(str == "D"):
        return "DASH"
    elif(str == "S"):
        return "PROT"
    elif(str == "F"):
        return "BONK"
    elif(str == "M"):
        return "GOLD"
    elif(str == "P"):
        return "SPED"
    else:
        print(str + ": unrecognized tile.",file=sys.stderr)
        assert 0

# ------------------------ parsing the data ------------------------ #

iii=0
with open(MAPNAME) as file:
    fst=True
    for line in file:
        line=line[:-1]      # remove annoying \n at the end
        if(fst):
            # initialize everything
            fst = False
            MAPLEN = int(line)
            PLAYER_SPAWN = [(1, 1), (MAPLEN-1-1, MAPLEN-1-1), (1, MAPLEN-1-1), (MAPLEN-1-1, 1)]
            PLAYER_SPAWN = PLAYER_SPAWN[:N_PLAYERS]     # only keeping players
            map = [["X" for _ in range(MAPLEN)] for _ in range(MAPLEN)]
        else:
            comp=line.split(" ")
            #print(comp)
            for jjj in range(len(comp)):
                data=comp[jjj].split(",")
                tType=(data[0] if len(data)==1 else data[1])
                if(isRelative(tType)):
                    map[iii][jjj] = (toTileName(data[0]),max(0,int(data[0])),0)
                else:
                    map[iii][jjj] = (toTileName(data[1]),int(data[0]),int(data[2]))
            iii+=1

for (x,y) in PLAYER_SPAWN:
    map[x][y] = ("RESO",0,0)

savex=[[0, 0] for i in range(MAPLEN)]
savey=[[0, 0] for i in range(MAPLEN)]
for i in range(MAPLEN):
    for k in range(2):
        tilex=map[i][MAPLEN//2-k]
        savex[i][k]=tilex[0]
        map[i][MAPLEN//2-k]=("WALL",tilex[1],tilex[2])
        if(i != MAPLEN//2 and i != MAPLEN//2-1):
            tiley=map[MAPLEN//2-k][i]
            savey[i][k]=tiley[0]
            map[MAPLEN//2-k][i]=("WALL",tiley[1],tiley[2])

isOpen=False
def enable_interactions(curTurn):
    global isOpen
    if(not isOpen and curTurn>=100):
        isOpen=True
        for i in range(MAPLEN):
            for k in range(2):
                tilex=map[i][MAPLEN//2-k]
                map[i][MAPLEN//2-k]=(savex[i][k],tilex[1],tilex[2])
                if(i != MAPLEN//2 and i != MAPLEN//2-1):
                    tiley=map[MAPLEN//2-k][i]
                    map[MAPLEN//2-k][i]=(savey[i][k],tiley[1],tiley[2])

TILE_SIZE=min(WIDTH//(MAPLEN+ADDTILE_R),HEIGHT//(MAPLEN+ADDTILE_B))
SP_OFFSET=TILE_SIZE/6
MN_OFFSET=TILE_SIZE/15

# text fonts for tkinter
DIG_FONT = ""
MIN_FONT = ""
MI2_FONT = ""
NAM_FONT = ""
LEA_FONT = ""
SCO_FONT = ""

def refreshCanvas(root,oldCanvas):
    oldCanvas.destroy()
    canvas = tk.Canvas(root, width=WIDTH, height=HEIGHT, bg="white")
    canvas.pack()

    return canvas

# some more graphics constants
LB_H = (MAPLEN-2)*TILE_SIZE
LB_W = 2*TILE_SIZE
LB_OFF = TILE_SIZE//10
WOFFS=150

# self explainatory
def drawMap(root,canvas,curTurn):
    canvas.create_rectangle(0,0,WIDTH,HEIGHT,fill="#dddddd")

    for i in range(N_PLAYERS):
        py,px=PLAYER_SPAWN[i]
        canvas.create_rectangle((px+1-2)*TILE_SIZE,(py+1-2)*TILE_SIZE,(px+1+3)*TILE_SIZE,(py+1+3)*TILE_SIZE,fill=PLAYER_COLOR[i])

    # map
    for x in range(MAPLEN):
        for y in range(MAPLEN):
            data=map[y][x]
            val = data[1]
            x0 = (1+x)*TILE_SIZE
            y0 = (1+y)*TILE_SIZE
            x1 = x0+TILE_SIZE
            y1 = y0+TILE_SIZE
            if data[0]=="WALL":
                # wall
                canvas.create_rectangle(x0,y0,x1,y1,fill="gray20")
            elif val>0:
                green = int(255*val/10)
                color = f"#00{green:02x}00"
                canvas.create_rectangle(x0,y0,x1,y1,fill=color)
            else:
                canvas.create_rectangle(x0,y0,x1,y1,fill="white")

            # special tiles
            if(data[0]=="DASH"):
                canvas.create_rectangle(x0+SP_OFFSET,y0+SP_OFFSET,x1-SP_OFFSET,y1-SP_OFFSET,fill="#bb11bb")
                canvas.create_text((x0+x1)//2,(y0+y1)//2,text=str(data[2]),fill="#ffffff",font=DIG_FONT)
            elif(data[0]=="PROT"):
                canvas.create_rectangle(x0+SP_OFFSET,y0+SP_OFFSET,x1-SP_OFFSET,y1-SP_OFFSET,fill="#0000ff")
                canvas.create_text((x0+x1)//2,(y0+y1)//2,text=str(data[2]),fill="#ffffff",font=DIG_FONT)
            elif(data[0]=="BONK"):
                canvas.create_rectangle(x0+SP_OFFSET,y0+SP_OFFSET,x1-SP_OFFSET,y1-SP_OFFSET,fill="#ff0000")
                canvas.create_text((x0+x1)//2,(y0+y1)//2,text=str(data[2]),font=DIG_FONT)
            elif(data[0]=="GOLD"):
                canvas.create_rectangle(x0+SP_OFFSET,y0+SP_OFFSET,x1-SP_OFFSET,y1-SP_OFFSET,fill="#c0c001")
                canvas.create_text((x0+x1)//2,(y0+y1)//2,text=str(data[2]),font=DIG_FONT)
            elif(data[0]=="SPED"):
                canvas.create_rectangle(x0+SP_OFFSET,y0+SP_OFFSET,x1-SP_OFFSET,y1-SP_OFFSET,fill="#25ffff")
                canvas.create_text((x0+x1)//2,(y0+y1)//2,text=str(data[2]),font=DIG_FONT)

    # special tiles
    canvas.create_rectangle(WIDTH-WOFFS,2*TILE_SIZE,WIDTH,3*TILE_SIZE,fill="#bb11bb")
    canvas.create_text(WIDTH-WOFFS//2,2.5*TILE_SIZE,text="DASH",fill="#ffffff",font=SCO_FONT)

    canvas.create_rectangle(WIDTH-WOFFS,3.5*TILE_SIZE,WIDTH,4.5*TILE_SIZE,fill="#0000ff")
    canvas.create_text(WIDTH-WOFFS//2,4*TILE_SIZE,text="PROT",fill="#ffffff",font=SCO_FONT)

    canvas.create_rectangle(WIDTH-WOFFS,5*TILE_SIZE,WIDTH,6*TILE_SIZE,fill="#ff0000")
    canvas.create_text(WIDTH-WOFFS//2,5.5*TILE_SIZE,text="BONK",font=SCO_FONT)

    canvas.create_rectangle(WIDTH-WOFFS,6.5*TILE_SIZE,WIDTH,7.5*TILE_SIZE,fill="#c0c001")
    canvas.create_text(WIDTH-WOFFS//2,7*TILE_SIZE,text="GOLD",font=SCO_FONT)

    canvas.create_rectangle(WIDTH-WOFFS,8*TILE_SIZE,WIDTH,9*TILE_SIZE,fill="#25ffff")
    canvas.create_text(WIDTH-WOFFS//2,8.5*TILE_SIZE,text="SPED",font=SCO_FONT)
                
    # data for each player
    for i in range(N_PLAYERS):
        py,px=PLAYER_SPAWN[i]
        canvas.create_rectangle((1+px)*TILE_SIZE,(1+py)*TILE_SIZE,(px+2)*TILE_SIZE,(py+2)*TILE_SIZE,fill=PLAYER_COLOR[i])
        dx=2*(1 if px>=MAPLEN//2 else -1)
        dy=2*(1 if py>=MAPLEN//2 else -1)
        canvas.create_text((1+px+3*dx/4)*TILE_SIZE+TILE_SIZE//2,(1+py+dy)*TILE_SIZE+TILE_SIZE//2,text=str(PLAYER_SCORE[i])+" pts",fill="#000000",font=SCO_FONT)
        canvas.create_text((1+px+dx)*TILE_SIZE+TILE_SIZE//2,(1+py+dy//2)*TILE_SIZE+TILE_SIZE//2,text=str(PLAYER_RSCS[i])+"$",fill="#000000",font=SCO_FONT)
        canvas.create_text((1+px-dx//2)*TILE_SIZE+TILE_SIZE//2,(1+py+dy)*TILE_SIZE+TILE_SIZE//2,text=PLAYER_NAMES[i],fill="#000000",font=NAM_FONT)

    # minions
    for i in range(N_PLAYERS):
        minionList=PLAYER_MINIONS[i]
        for (x,y),(cap,hp,size,atk) in minionList.items():
            x0 = (1+y)*TILE_SIZE
            y0 = (1+x)*TILE_SIZE
            x1 = x0+TILE_SIZE
            y1 = y0+TILE_SIZE

            canvas.create_rectangle(x0+MN_OFFSET,y0+MN_OFFSET,x1-MN_OFFSET,y1-MN_OFFSET, fill=PLAYER_COLOR[i])
            canvas.create_text((x0+x1)//2, (y0+y1)//2-TILE_SIZE//3,text=str(cap*(1+addCapa))+"/"+str(size),font=MIN_FONT)
            canvas.create_text((x0+x1)//2, (y0+y1)//2             ,text=str(atk*(1+addDMG))+" DMG",font=MI2_FONT)
            canvas.create_text((x0+x1)//2, (y0+y1)//2+TILE_SIZE//3,text=str(hp*(1+addHP))+" HP",font=MI2_FONT)

    # metadata
    # current turn
    canvas.create_text(TILE_SIZE*(2+MAPLEN)//2,TILE_SIZE//2,text=str(curTurn)+"/"+str(MAX_TURNS),font=MIN_FONT,fill="#222222")

    # random event
    canvas.create_text(TILE_SIZE*(2+MAPLEN)//2,(1+MAPLEN)*TILE_SIZE+TILE_SIZE//2,text="Event : "+currentRandomEvent,font=MIN_FONT,fill="#222222")
    if(randomEventList != []):
        canvas.create_text(TILE_SIZE*(2+MAPLEN)//2,(1+MAPLEN)*TILE_SIZE+TILE_SIZE,text="Next  : "+randomEvents[randomEventList[0]]["name"],font=MIN_FONT,fill="#222222")

    # leaderboard
    maxStat=max(1,max([x+y for x, y in zip(PLAYER_SCORE, PLAYER_CARRY)]))
    for p in range(N_PLAYERS):
        bH = max(1,(LB_H*PLAYER_SCORE[p])//maxStat)
        bI = max(0,(LB_H*PLAYER_CARRY[p])//maxStat)
        x0 = (3+2*p+MAPLEN)*TILE_SIZE
        y0 = 2*TILE_SIZE+(LB_H-bH-bI)

        canvas.create_rectangle(
            x0,
            y0,
            x0+LB_W,
            y0+bH+bI,
            fill="#222222")
        canvas.create_rectangle(
            x0+LB_OFF,
            y0+bI,
            x0+LB_W-LB_OFF,
            y0+bH+bI,
            fill=PLAYER_COLOR[p])
        canvas.create_rectangle(
            x0+LB_OFF,
            y0,
            x0+LB_W-LB_OFF,
            y0+bI,
            fill=PLAYER_COLOR_2[p])
        canvas.create_text(
            x0+LB_W//2,
            LB_OFF+y0+bI-TILE_SIZE//2,
            text=str(PLAYER_SCORE[p]),
            fill="#000000",font=LEA_FONT)
        if(PLAYER_CARRY[p] > 0):
            canvas.create_text(
                x0+LB_W//2,
                LB_OFF+y0-TILE_SIZE//2,
                text="+"+str(PLAYER_CARRY[p]),
                fill="#333333",font=LEA_FONT)

# -------------------------| random events |------------------------- #
randomEventQueue=[]     # this must be sorted in increasing order at all times

def nothing():
    # a function that does nothing
    pass

def reset_all():
    global addDMG
    global addHP
    global addCapa
    global addPump
    global freePump
    addDMG,addHP,addCapa,addPump,freePump=0,0,0,0,0

def hello_i_like_money():
    # money galore event
    for i in range(MAPLEN):
        for j in range(MAPLEN):
            tile=map[i][j]
            map[i][j] = (tile[0],min(10,tile[1]+2),tile[2])

def thats_a_lot_of_damage():
    global addDMG
    addDMG+=1

def protection_iv():
    global addHP
    addHP+=1

def quantum_storage():
    global addCapa
    addCapa+=1

def america():
    global addPump
    addPump+=1

def free_real_estate():
    global freePump
    freePump=1

randomEvents=[{
    "name":"MoneyGalore",
    "weight":10,
    "lastingTurns":-1,
    "onTrigFct":hello_i_like_money,
    "onEndFct":nothing
},{
    "name":"bonking",
    "weight":10,
    "lastingTurns":20,
    "onTrigFct":thats_a_lot_of_damage,
    "onEndFct":reset_all
},{
    "name":"cant_touch_me",
    "weight":10,
    "lastingTurns":20,
    "onTrigFct":protection_iv,
    "onEndFct":reset_all
},{
    "name":"more_storage",
    "weight":10,
    "lastingTurns":20,
    "onTrigFct":quantum_storage,
    "onEndFct":reset_all
},{
    "name":"extraction",
    "weight":10,
    "lastingTurns":20,
    "onTrigFct":america,
    "onEndFct":reset_all
},{
    "name":"free pump",
    "weight":10,
    "lastingTurns":20,
    "onTrigFct":free_real_estate,
    "onEndFct":reset_all
}]
''' list of dicts -> {name, weight, lastingTurns, onTrigFct, onEndFct} 
    Functions take nothing as arguments and return None '''

def initRandomEvents():
    nEvents=MAX_TURNS//150
    for _ in range(nEvents+1):
        randomEventList.append(random.randint(0,len(randomEvents)-1))
        randomEventFullList.append(randomEventList[-1])

def triggerRandomEvent(curTurn):
    global currentRandomEvent
    if(curTurn%150 == 0):
        # one event every 150 turns
        id=randomEventList.pop(0)
        randomEvents[id]["onTrigFct"]()
        currentRandomEvent=randomEvents[id]["name"]
        randomEventQueue.append((randomEvents[id]["lastingTurns"],randomEvents[id]["onEndFct"]))

def popREQueue():
    global currentRandomEvent
    if(randomEventQueue != []):
        (rem,fct)=randomEventQueue[0]
        if(rem<=0):
            fct()
            currentRandomEvent=""
            randomEventQueue.pop(0)
        else:
            randomEventQueue[0]=(rem-1,fct)

# -------------------------| MainLoop |------------------------- #
# control over simulation speed
def onKeyPress(event):
    global DT
    global HALT
    #print(f"You pressed {event.char}")
    if(event.char == "+"):
        DT = max(0.02, DT-0.02)
    elif(event.char == "-"):
        DT = min(2.0, DT+0.02)
    elif(event.char == " "):
        HALT = not HALT

turnOrder = [i for i in range(N_PLAYERS)]
def mainLoop():
    global DIG_FONT
    global MIN_FONT
    global MI2_FONT
    global NAM_FONT
    global LEA_FONT
    global SCO_FONT
    global root

    currentTurn=0
    root.title("Moteur Screeps simplifié")

    root.bind('<KeyPress>', onKeyPress)

    sizeMult=14/MAPLEN
    DIG_FONT = tk.font.Font(family = "Symbol", size = int(sizeMult*24))
    MIN_FONT = tk.font.Font(family = "monospace", size = int(15*sizeMult))
    MI2_FONT = tk.font.Font(family = "monospace", size = int(sizeMult*13))
    LEA_FONT = tk.font.Font(family = "monospace", size = int(sizeMult*30))
    NAM_FONT = tk.font.Font(family = "Arial", size = int(sizeMult*12))
    SCO_FONT = tk.font.Font(family = "Bold", size = int(sizeMult*20))
    HAL_FONT = tk.font.Font(family = "Bold", size = 140)

    canvas = tk.Canvas(root, width=WIDTH, height=HEIGHT, bg="white")
    canvas.pack()

    drawMap(root,canvas,currentTurn)
    root.update()

    thread = threading.Thread(target=playsound)
    thread.start()

    initRandomEvents()
    while(currentTurn < MAX_TURNS):
        st = time.time()
        # turn order is randomized
        random.shuffle(turnOrder)
        for p in turnOrder:
            execute_player(p,currentTurn)
            # kill
            killDeadMinions()

        # log the end of the turn + update the canvas
        print("",file=logFile)
        if(currentTurn%16==15):
            canvas = refreshCanvas(root,canvas)
        
        drawMap(root,canvas,currentTurn)
        while(HALT):
            time.sleep(0.4)
            canvas.create_text(WIDTH//2,HEIGHT//2,text="PAUSED",fill="#000000",font=HAL_FONT)
            root.update()

        root.update()

        # sleep
        et = time.time()
        time.sleep(max(0.0, DT-(et-st)))

        # end the turn
        # print(et-st)
        currentTurn += 1
        popREQueue()
        triggerRandomEvent(currentTurn)
        enable_interactions(currentTurn)

    hstat = HALT
    canvas.create_text(WIDTH//2,HEIGHT//2,text="GAME OVER",fill="#000000",font=HAL_FONT)
    while(HALT == hstat):
        root.update()

    logFile.close()

mainLoop()