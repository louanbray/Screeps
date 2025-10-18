import sys
import random
import math

'''
argument 1 : the map's file name
argument 2 : the size of a quadrant of the map (square)
argument 3 : the average value of a cell (can be a float)
argument 4 : the standard deviation of the values of a cell (gaussian distribution)
argument 5 : the value buff towards the center (avg at the center is this_value + avg_value)
argument 6 : the percentage of special tiles
argument 7 : the max distance to the center for special tiles to start spawning (between 0 and N-1)
'''

try:
    # parse everything
    _,filename,N,avg,std,midbuff,sp,spDist = sys.argv
    N,avg,std,midbuff,sp,spDist = int(N),float(avg),float(std),float(midbuff),float(sp),int(spDist)
    
    # build walls (all coords here should be between 0 and N-1)
    #walls = [[4, 0], [4, 1], [0, 4], [1, 4]]
    walls = [[3,3], [2,3], [3,2]]

    # out file
    f=open(filename,"w")

    map = [["-1" for _ in range(2*N)] for _ in range(2*N)]

    # distribution of special tiles
    spWeights = [20, 20, 20, 20, 20]
    totalWeight = 100

    # a function used to randomly generate the values of the cells
    # this can be changed depending on context
    def boxMuller(i,j):
        U = random.randint(1,1000)/1000
        V = random.randint(1,1000)/1000
        toMidDist = 1-(abs(i-N) + abs(j-N))/(2*N)
        return (math.sqrt(std)*math.sqrt(-2*math.log(U)) * math.cos(2*3.14159265358979323*V) + avg + toMidDist*midbuff)
        ''' if you're a spé seeing this, you now have to prove that this function behaves like a gaussian law :) '''

    # build the map
    for line in range(N):
        for col in range(N):
            spVal = random.randint(1,10000)/100
            value="-1"
            if not ([line, col] in walls):
                value = str(min(10,max(1,int(boxMuller(line,col)))))
                
            if ((abs(line-N) <= spDist and abs(col-N) <= spDist) and (spVal < sp)):
                weight=random.randint(0,100)
                sum=spWeights[0]
                i=0
                while(weight > sum):
                    i+=1
                    sum += spWeights[i]
                    
                if(i==0):       # dash panel
                    value=value+","+"D,"+str(random.randint(1,4))
                    
                elif(i==1):     # shield
                    value=value+",""S,"+str(random.randint(2,6))
                    pass
                
                elif(i==2):     # strength
                    value=value+",""F,"+str(random.randint(1,3))
                    pass
                
                elif(i==3):     # midas
                    value=value+",""M,"+str(random.randint(3,5))
                    pass
                
                elif(i==4):     # speed
                    value=value+",""P,"+str(random.randint(3,6))
                    pass
                
                else:
                    print(f"Unsupported special type {i}",file=sys.stderr)
                    assert 0
                
            map[line      ][      col] = value
            map[line      ][2*N-1-col] = value
            map[2*N-1-line][      col] = value
            map[2*N-1-line][2*N-1-col] = value


    # output the map
    print(2*N,file=f)
    for i in range(len(map)):
        line=map[i]
        for j in range(len(line)):
            elt=line[j]
            print(elt,end=(" " if j != 2*N-1 else ""),file=f)
        
        print("",file=f)

    print(f"generated map of size {2*N}x{2*N} inside {filename}")
    f.close()
except:
    print("Usage : python3 gen__.py <filename> <quadSize> <avgValue> <stdev> <midAvgIncr> <specialRate>",file=sys.stderr)
    print("For fixed walls, you may change the \"generateWalls()\" function.",file=sys.stderr)
    assert 0    # to still see what caused an exception