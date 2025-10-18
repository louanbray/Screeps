# ----------------------------------- #
# Specifications for player functions #
# ----------------------------------- #
# Please note that there can be at most 1 minion per tile

**INPUT** (to be read inside *mapData.txt*) :
    - N : the size of the (square) map
    - next N lines : N values separated by a space, with the format <T>,<N>,<D> where :
            \* <T> is the type of the tile (R for resource only, W for a wall, anything else for a special tile)
            \* <N> is the number of resources on that tile
            \* <D> is data relative to the special tile (see *special.md* for full documentation) (0 if it's a resource tile)
    - M : the number of active minions on the field
    - next M lines : data regarding the players' minions
        format is <P>,<X>,<Y>,<CAR>,<HP>,<SIZE>,<ATK> where :
            \* <P> is the id of the player who owns the minion
            \* <X> and <Y> are its coords (please note that (0,0) is at the top left of the map)
            \* <CAR> is the number of resource the minion is holding
            \* <HP>,<SIZE>,<ATK> are the minion's stats
    - ID RSC : your player ID and the current number of resources you have
    - SPX SPY : the coordinate of your depot
    - <currentTurn> <maxTurn>
    - a list of couples <id>,<turn> :  
        \* these are special events that will trigger at turn <turn>. The list of special events is found inside *randomEvents.md*  

**OUTPUT** (to be writen inside *answer.txt*) :
    - at most K lines (where K is the number of the minion you own) with the following structure :
        **<X>,<Y>,<XDEST>,<YDEST>**
            \* <X>,<Y> is the coords of the selected minion
            \* <XDEST>,<YDEST> is the desired destinaton :
                -> if XDEST,YDEST==X,Y, then the action will be to *PUMP* one resource from the tile you're standing on
                -> if XDEST,YDEST is exactly one tile away from your X,Y, you will move here
                    \* moving into an enemy minion will cause you to attack him (if you survive and kill you will not move to his location)
                    \* moving into a friendly minion/your depot will transfer your reserve of resources into the target (the max amount will be transfered)
    - (optionnal) one line at the end with the following structure :
        CREATE <HP>,<SIZE>,<ATK>
            \* this will create a minion at your depot with the corresponding stats. If you do not have enough points to create one, this instruction will be
            ignored