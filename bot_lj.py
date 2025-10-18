#! /usr/bin/python3
from random import choice

CASE_TYPE = 0
CASE_RESS = 1
CASE_SPEC = 2

TYPE_MUR = 'W'
TYPE_NORMAL = 'R'
TYPE_DASH = 'D'
TYPE_SHIELD = 'S'
TYPE_FORCE = 'F'
TYPE_MIDAS = 'M'
TYPE_VITESSE = 'P'
TYPE_MOI = 'I'
TYPE_ENNEMI = 'E'

MINION_PROP = 0
MINION_X = 1
MINION_Y = 2
MINION_CARG = 3
MINION_HP = 4
MINION_CARG_MAX = 5
MINION_ATTAQUE = 6

with open("mapData.txt") as mapFile:
    
    n = int(mapFile.readline())

    carte = [[None]*n for i in range(n)]
    for i in range(n):
        ligne = mapFile.readline()
        for j,case in enumerate(ligne.strip().split()):
            info = case.split(",")
            carte[i][j] = [info[0], int(info[1]), int(info[2])]

    m = int(mapFile.readline())
    
    minionsJoueurs = {}
    for i in range(m):
        *minion, = map(int, mapFile.readline().strip().split(","))
        if minion[MINION_PROP] in minionsJoueurs:
            minionsJoueurs[minion[MINION_PROP]].append(minion[1:])
        else:
            minionsJoueurs[minion[MINION_PROP]]=[minion[1:]]
        
    id, ressActuelles = map(int, mapFile.readline().strip().split())
    depotX, depotY = map(int, mapFile.readline().strip().split())

    if id in minionsJoueurs:
        mesMinions = minionsJoueurs[id]
    else:
        mesMinions = []
    for jid in minionsJoueurs:
        for mX, mY, mCarg, mHp, mCargMax, mForce in minionsJoueurs[jid]:
            carte[mX][mY][CASE_TYPE] = TYPE_MOI if jid == id else TYPE_ENNEMI

    def enleve_impossibles(cands):
        *cands, = filter(lambda c:c[0]>=0 and c[1]>=0 and c[0]<n and c[1]<n and carte[c[0]][c[1]][CASE_TYPE]!=TYPE_MUR, cands)
        return cands

    def choix_aleatoire(mX, mY, cands=None):
        if cands!=None:
            cands = enleve_impossibles(cands)
        if cands==None or cands==[]:
            cands = [(mX-1, mY), (mX+1, mY), (mX, mY-1), (mX, mY+1)]
        cands = enleve_impossibles(cands)
        if cands==[]:
            return mX, mY
        else:
            return choice(cands)

    with open("answer.txt", "w") as reponse:
        mesMinions.sort(key = lambda m: abs(m[MINION_X]-depotX)+abs(m[MINION_Y]-depotY), reverse=True)
        for mX, mY, mCarg, mHp, mCargMax, mForce in mesMinions:
            if mCargMax > 0: # pas un soldat
                if mCarg == mCargMax:
                # Retour en direction du dépôt
                    cands = []
                    if mX < depotX:
                        cands.append([mX+1, mY])
                    elif mX > depotX:
                        cands.append([mX-1, mY])
                    if mY < depotY:
                        cands.append([mX, mY+1])
                    elif mY > depotY:
                        cands.append([mX, mY-1])
                    newX, newY = choix_aleatoire(mX, mY, cands)
                    carte[mX][mY][CASE_TYPE] = TYPE_NORMAL
                    carte[newX][newY][CASE_TYPE] = TYPE_MOI
                    print(mX, mY, newX, newY, file=reponse)
                else:
                    if carte[mX][mY][CASE_RESS]>0:
                        print(mX, mY, mX, mY, file=reponse) # Pomper
                    else:
                        cands = [(mX-1, mY), (mX+1, mY), (mX, mY-1), (mX, mY+1)]
                        cands = enleve_impossibles(cands)
                        choix = max(zip(map(lambda c:carte[c[0]][c[1]][CASE_RESS], cands), cands))
                        if choix[0] == 0:
                            newX, newY = choix_aleatoire(mX, mY)
                        else:
                            newX, newY = choix[1]
                        carte[mX][mY][CASE_TYPE] = TYPE_NORMAL
                        carte[newX][newY][CASE_TYPE] = TYPE_MOI
                        print(mX, mY, newX, newY, file=reponse) # Se déplacer
        if ressActuelles>4:
            print("CREATE", 1, min(ressActuelles-1,10), 0, file=reponse)