#include "software_renderer.cpp"
#include "collision.cpp"

#define BALL_ACTIVE 0x1
#define BALL_DESTROYED_ON_DP_Y_DOWN 0x2


struct {
    v2 P;
    v2 dP;
    v2 Size;
    u32 Color;

    v2 VisualP;
    v2 VisualDp;
    v2 VisualSize;
} typedef paddle;

struct {
    v2 P;
    real32 Life;
} typedef ball_trail;

struct {
    u32 Flags;
    v2 P;
    v2 dP;
    v2 Size;
    v2 DesiredP;
    real32 BaseSpeed;
    u32 Color;

    ball_trail Trails[64];
    u32 NextTrail;
    s32 TrailSpawner;
} typedef ball;

struct {
    v2 P;
    v2 Size;
} typedef arena;

enum {
    LEVEL_01_NORMAL,
    LEVEL_02_WALL,

    LEVEL_05_PONG,
    LEVEL_06_INVADERS,

    LEVEL__COUNT,

    LEVEL_03_STADIUM,
    LEVEL_04_CHESS,
} typedef level;

enum {
    POWER_INACTIVE,
    POWER_INVINCIBILITY,
    POWER_TRIPLE_SHOT,
    POWER_COMET,

    POWER_LIMIT_BETWEEN_BAD_AND_GOOD,

    POWER_INSTAKILL,
    POWER_STRONG_BLOCKS,
    POWER_INVERTED_CONTROLS,
    POWER_SLOW_PLAYER,

    POWER__COUNT
} typedef power_type;

struct {
    power_type Type;
    v2 P;
} typedef power;

typedef struct {
    v2 P;
    v2 Size;
    v2 DistanceFromCenter;

    s32 Life;
    u32 Color;

    power Power;
} block;

typedef struct {
    s32 EnemyLife;
    v2 EnemyP;
    v2 EnemyDp;
    v2 EnemySize;
} level_state_pong;

typedef struct {
    v2 CenterP;
    v2 dP;
    v2 InvadersFullSize;
} level_state_invaders;

typedef struct {
    union {
        level_state_pong Pong;
        level_state_invaders Invaders;
    };
} game_state;


#include "console.cpp"

global_variable game_state GlobalGameState;

#define BLOCK_COUNT 1024 * 2
global_variable block BlockList[BLOCK_COUNT];

global_variable bool32 Initialized;
global_variable paddle Paddle;
// global_variable ball Ball;
global_variable arena Arena;
// global_variable real32 Speed;
global_variable bool32 FirstBallMovement;
global_variable level GlobalCurrentLevel;
global_variable s32 BlockTotalCount;
global_variable bool32 AdvanceGameMode;
global_variable ball Balls[16];
global_variable s32 GlobalNumberOfTripleShots;
global_variable real32 InvincibilityTime;
global_variable real32 CometTime;
global_variable real32 StrongBlocksTime;
global_variable real32 InvertedControlsTime;

// global_variable game_mode_state GameModeState;
global_variable power PowerBlocks[16];
global_variable s32 NextPowerBlock;
global_variable v2 PowerBlockSize;

global_variable v2 PaddleDesiredP;
global_variable s32 PlayerLifeCount;
global_variable real32 GlobalBallBaseSpeed = 40;
global_variable s32 GlobalScore;

global_variable s32 GlobalBlockIndex;

global_variable game_render_buffer *GlobalBuffer_DEBUG;



void
CreateInvader(v2 P, real32 SizeX, real32 SizeY, v2 CenterP) {
#define INVADER_ROW_CHAR_COUNT 11
#define INVADER_COLUMN_CHAR_COUNT 8 
    char InvaderTemplate[][INVADER_ROW_CHAR_COUNT + 1] = {
        " 0       0 ",
        "  0     0  ",
        "  0000000  ",
        " 00 000 00 ",
        "00000000000",
        "0 0000000 0",
        "0 0     0 0",
        "   00 00   ",
    };

    // real32 InvaderWidth = Size;
    // real32 InvaderHeight = Size * INVADER_COLUMN_CHAR_COUNT / INVADER_ROW_CHAR_COUNT;
    real32 InvaderWidth = SizeX;
    real32 InvaderHeight = SizeY;
    real32 BlockSize = InvaderWidth / INVADER_ROW_CHAR_COUNT;


    for(int YIndex = 0; YIndex < INVADER_COLUMN_CHAR_COUNT; YIndex++){
        for(int XIndex = 0; XIndex < INVADER_ROW_CHAR_COUNT; XIndex++) {
            char Value = InvaderTemplate[YIndex][XIndex];
            if(Value == '0') {
                BlockTotalCount++;
                block *Block = BlockList + GlobalBlockIndex++;
                if(GlobalBlockIndex >= BLOCK_COUNT) { GlobalBlockIndex = 0; }
                Block->Life = 1;
                Block->Size.X = BlockSize;
                Block->Size.Y = BlockSize;
                Block->P.X = P.X - (InvaderWidth/2) + (XIndex*BlockSize);
                Block->P.Y = P.Y - (InvaderHeight/2) + (YIndex*BlockSize);
                Block->DistanceFromCenter.X = Block->P.X - CenterP.X;
                Block->DistanceFromCenter.Y = Block->P.Y - CenterP.Y;
            }
        }
    }
}

void
InitBall(ball *Ball) 
{
    Ball->BaseSpeed = GlobalBallBaseSpeed;
    Ball->P = V2(10, -50);
    Ball->dP = V2(0.0, -Ball->BaseSpeed);
    Ball->Size = V2(1.5, 1.5);
    Ball->Color = 0x00FFFFFF;
    Ball->Flags |= BALL_ACTIVE;
}

internal void 
StartGame(level GameMode)
{
    // Speed = 30.0f;
    PlayerLifeCount = 3;

    GlobalCurrentLevel = GameMode;
    AdvanceGameMode = false;

    FirstBallMovement = 1;

    InitBall(&Balls[0]);

    int ArenaWidth = 160;
    int ArenaHeight = 100;
    Arena.P = V2(0, 5);
    Arena.Size = V2(ArenaWidth, ArenaHeight);

    Paddle.Size = V2(20, 4);
    Paddle.VisualSize = V2(Paddle.Size.X, Paddle.Size.Y);

    Paddle.P.Y = 45;
    Paddle.Color = 0x0000ff00;
    Paddle.VisualP = V2(Paddle.P.X, Paddle.P.Y);

    s32 GlobalBlockIndex = 0;

    float BlocksFullWidth = ArenaWidth - 10;
    float BlocksFullHeight = ArenaHeight - 10;

    BlockTotalCount = 0;

    for(s32 Index = 0; Index < BLOCK_COUNT; Index++) {
        block *Block = BlockList + Index;
        Block->Life = 0;
    }

    switch(GameMode)
    {
        case LEVEL_01_NORMAL:
        {
            s32 BlockXCount = 19;
            s32 BlockYCount = 9;

            s32 YColorStep = 255 / BlockYCount;
            s32 XColorStep = 255 / BlockXCount;

            real32 BlockOffsetX = 0.4;
            real32 BlockOffsetY = 0.4;

            real32 BlockSizeX = (BlocksFullWidth / BlockXCount) - BlockOffsetX;

            real32 StartX = -BlocksFullWidth/2 + ((BlockSizeX+BlockOffsetX)/2);
            real32 StartY = -ArenaHeight/2 + 10;

            for(s32 Y = 0; Y < BlockYCount; Y++) {
                for(s32 X = 0; X < BlockXCount; X++) {
                    BlockTotalCount++; 
                    block *Block = BlockList + GlobalBlockIndex++;
                    if(GlobalBlockIndex >= BLOCK_COUNT) { GlobalBlockIndex = 0; }
                    Block->Life = 1;
                    Block->Color = (Y*YColorStep << 8) | 
                                   (X*XColorStep);
                    Block->Size.X = BlockSizeX;
                    Block->Size.Y = 3;
                    Block->P.X = StartX + X * (Block->Size.X + BlockOffsetX);
                    Block->P.Y = StartY + Y * (Block->Size.Y + BlockOffsetY);
                }
            }
        } break;

        case LEVEL_02_WALL:
        {
            s32 BlockXCount = 19;
            s32 BlockYCount = 12;

            s32 YColorStep = 255 / BlockYCount;
            s32 XColorStep = 1;

            real32 BlockSizeX = (BlocksFullWidth / BlockXCount);

            real32 StartX = -BlocksFullWidth/2 + (BlockSizeX/2);
            real32 StartY = Arena.P.Y - Arena.Size.Y/2 + 10;

            for(s32 Y = 0; Y < BlockYCount; Y++) {
                for(s32 X = 0; X < BlockXCount; X++) {
                    BlockTotalCount++; 
                    block *Block = BlockList + GlobalBlockIndex++;
                    if(GlobalBlockIndex >= BLOCK_COUNT) { GlobalBlockIndex = 0; }
                    Block->Life = 1;
                    Block->Color = (Y*YColorStep << 8) | 
                                   (X*XColorStep);
                    Block->Size.X = BlockSizeX;
                    Block->Size.Y = 3;
                    Block->P.Y = StartY + Y * (Block->Size.Y);
                    Block->P.X = StartX + X * (Block->Size.X);
                }
            }
        } break;

        case LEVEL_03_STADIUM:
        {
        } break;

        case LEVEL_04_CHESS:
        {
        } break;

        case LEVEL_05_PONG:
        {
            s32 BlockXCount = 15;
            s32 BlockYCount = 3;

            s32 YColorStep = 255 / BlockYCount;
            s32 XColorStep = 1;

            float BlocksFullWidth = (ArenaWidth / 5);
            real32 BlockSizeX = (BlocksFullWidth / BlockXCount);

            real32 FromCenterStartOffsetX = -(BlocksFullWidth / 2);

            real32 StartX = -BlocksFullWidth/2 + (BlockSizeX/2);
            real32 StartY = Arena.P.Y - Arena.Size.Y/2 + 10;

            for(s32 Y = 0; Y < BlockYCount; Y++) {
                for(s32 X = 0; X < BlockXCount; X++) {
                    BlockTotalCount++; 
                    block *Block = BlockList + GlobalBlockIndex++;
                    if(GlobalBlockIndex >= BLOCK_COUNT) { GlobalBlockIndex = 0; }
                    Block->Life = 1;
                    Block->Color = (Y*YColorStep << 8) | (X*XColorStep);
                    Block->Size.X = BlockSizeX;
                    Block->Size.Y = 3;
                    Block->P.Y = StartY + Y * (Block->Size.Y);
                    Block->P.X = StartX + X * (Block->Size.X);

                    Block->DistanceFromCenter.X = FromCenterStartOffsetX + (X * BlockSizeX);
                    // Block->DistanceFromCenter.Y = ;
                }
            }


        } break;

        case LEVEL_06_INVADERS:
        {
            level_state_invaders *LevelInvaders = &GlobalGameState.Invaders; 
            LevelInvaders->dP = V2(0.05, 0);

            GlobalBlockIndex = 0;

            s32 PaddingX = 2;
            s32 PaddingY = 2;

            s32 InvaderCountX = 5;
            s32 InvaderCountY = 3;

            real32 InvadersTotalWidth = (ArenaWidth / 2);
            real32 WidthToHeightMultiplier = ((float)INVADER_COLUMN_CHAR_COUNT / (float)INVADER_ROW_CHAR_COUNT);
            real32 InvadersTotalHeight = InvadersTotalWidth * WidthToHeightMultiplier;

            real32 InvaderSizeX = (InvadersTotalWidth / InvaderCountX) - PaddingX;
            real32 InvaderSizeY = (InvaderSizeX * WidthToHeightMultiplier) - PaddingY;

            LevelInvaders->InvadersFullSize = V2(InvadersTotalWidth, InvadersTotalHeight);

            LevelInvaders->CenterP = V2(
                    -(Arena.Size.X/2) + InvadersTotalWidth/2, 
                    (Arena.Size.Y/2) - InvadersTotalHeight);

            v2 CenterP = LevelInvaders->CenterP;

            for(s32 YIndex = 0; YIndex < InvaderCountY; YIndex++) {
                for(s32 XIndex = 0; XIndex < InvaderCountX; XIndex++) {
                    real32 X = CenterP.X - (InvadersTotalWidth/2) + (XIndex * InvaderSizeX) + (XIndex * PaddingX) + (InvaderSizeX/2);
                    real32 Y = CenterP.Y - (InvadersTotalHeight/2) + (YIndex * InvaderSizeY) + (YIndex * PaddingY);
                    // v2 DistanceFromCenter = {
                    //     CenterP.X - (InvadersTotalWidth/2*XIndex),
                    //     CenterP.Y - (InvadersTotalHeight/2*YIndex),
                    // };
                    CreateInvader(V2(X, Y), InvaderSizeX, InvaderSizeY, CenterP);
                }
            }
        } break;

        default: {
        };
    }
}

void
SimulateGameMode(game_render_buffer *Buffer)
{
    for(s32 Index = 0; Index < ARRAY_COUNT(PowerBlocks); Index++) {
        power *Power = &PowerBlocks[Index];
        if(Power->Type == POWER_INACTIVE) {
            continue;
        }

        if(Power->Type < POWER_LIMIT_BETWEEN_BAD_AND_GOOD) {
            DrawRect(Buffer, Power->P, V2(2, 2), 0xf05454);
        } else {
            DrawRect(Buffer, Power->P, V2(2, 2), 0xffff00);
        }
    }

    switch(GlobalCurrentLevel){
        case LEVEL_02_WALL: 
        {
        } break;

        default: { }
    }

}

void
SimulateLevelStateChanges(game_render_buffer *Buffer, level CurrentLevel, v2 PaddleP) {
   switch(CurrentLevel){
       case LEVEL_05_PONG: 
       {
           level_state_pong *Pong = &GlobalGameState.Pong;

           ball *Ball = Balls;

           if(Pong->EnemyP.X < Ball->P.X) { Pong->EnemyDp.X = 0.2; }
           else                           { Pong->EnemyDp.X = -0.2; }

           Pong->EnemyP.X += Pong->EnemyDp.X; 

           for(s32 Index = 0; Index < ARRAY_COUNT(BlockList); Index++) {
               block *Block = BlockList + Index;
               Block->P.X = Pong->EnemyP.X + Block->DistanceFromCenter.X;
           }
       } break;

       case LEVEL_06_INVADERS: 
       {
           level_state_invaders *LevelInvaders = &GlobalGameState.Invaders; 
           LevelInvaders->CenterP = LevelInvaders->CenterP + LevelInvaders->dP;

           real32 InvaderLeftMargin  = LevelInvaders->CenterP.X - LevelInvaders->InvadersFullSize.X/2;
           real32 InvaderRightMargin = LevelInvaders->CenterP.X + LevelInvaders->InvadersFullSize.X/2;

           if(InvaderLeftMargin < (-Arena.Size.X/2) ) {
               LevelInvaders->dP.X *= -1;
               LevelInvaders->CenterP.Y += 5;
           }else if(InvaderRightMargin > (Arena.Size.X/2)) {
               LevelInvaders->dP.X *= -1;
               LevelInvaders->CenterP.Y += 5;
           }

           for(s32 Index = 0; Index < ARRAY_COUNT(BlockList); Index++) {
               block *Block = BlockList + Index;
               Block->P.X = LevelInvaders->CenterP.X + Block->DistanceFromCenter.X;
               Block->P.Y = LevelInvaders->CenterP.Y + Block->DistanceFromCenter.Y;

               if(COLLISION_NONE != CollisionSide(Block->P, Block->Size, PaddleDesiredP, Paddle.VisualSize)) {
                   StartGame(GlobalCurrentLevel);
                   printf("Collided\n");
               }

           }

           DrawRect(Buffer, V2(InvaderLeftMargin, -25), V2(0.1, 3), 0x00ffffff);
           DrawRect(Buffer, LevelInvaders->CenterP, V2(0.1, 3), 0x00ffffff);
           DrawRect(Buffer, V2(InvaderRightMargin, -25), V2(0.1, 3), 0x00ffffff);
       }
       
       default: {} break;
   }
}

void
SpawnPowerup(v2 P, power_type Type) 
{
    power *Power = PowerBlocks + NextPowerBlock;
    NextPowerBlock++;
    if(NextPowerBlock >= ARRAY_COUNT(PowerBlocks)) {
        NextPowerBlock = 0;
    }
    Power->P = P;
    Power->Type = Type;
    // Power->Type = POWER_INVINCIBILITY;
    // Power->Type = POWER_TRIPLE_SHOT;
    // Power->Type = POWER_COMET;
    // Power->Type = POWER_INSTAKILL;
    // Power->Type = POWER_STRONG_BLOCKS;
    // Power->Type = POWER_INVERTED_CONTROLS;
}

void
TestForWinCondition()
{
    BlockTotalCount--; 
    if(BlockTotalCount <= 0) {
        AdvanceGameMode = true;
    }
}

void
BlockIsDestroyed(block *Block)
{
    TestForWinCondition();

    // SpawnPowerup(Block->P);

    power_type Type = POWER_TRIPLE_SHOT;

    switch(GlobalCurrentLevel) {
        case LEVEL_01_NORMAL: 
        {
            SpawnPowerup(Block->P, POWER_INVINCIBILITY);
        } break;

        case LEVEL_02_WALL: 
        {
            SpawnPowerup(Block->P, POWER_TRIPLE_SHOT);
        } break;

        default: 
        {
        } break; 
    }
}




void
SpawnTripleShotsBalls(v2 Origin)
{
    ball *Ball = 0;
    for(s32 i = 0; i < 2; i++) {
        for(ball *TempBall = Balls; TempBall != Balls + ARRAY_COUNT(Balls); TempBall++) {
            if(!(TempBall->Flags & BALL_ACTIVE)) {
                Ball = TempBall;
            }
        }

        if(Ball) {
            ZERO_STRUCT(*Ball);
            Ball->P.X = Origin.X;
            Ball->P.Y = Origin.Y;
            Ball->Size = V2(1,1);
            Ball->Color = 0xcccccc;
            Ball->Flags = BALL_ACTIVE | BALL_DESTROYED_ON_DP_Y_DOWN;
            Ball->dP.Y = -GlobalBallBaseSpeed;
            Ball->dP.X = GlobalBallBaseSpeed;
        }
    }
    if(Ball) {
        Ball->dP.X = -GlobalBallBaseSpeed;
    }
}

void 
ProcessBallOnDpYDown(ball *Ball) 
{
    if(Ball->Flags & BALL_DESTROYED_ON_DP_Y_DOWN) {
        Ball->Flags &= ~BALL_ACTIVE;
    }
}

void
LoseLife()
{
    PlayerLifeCount--;
    if(PlayerLifeCount <= 0) { StartGame(GlobalCurrentLevel); }

    FirstBallMovement = 1;
    InitBall(&Balls[0]);
}

void
DrawTest(game_render_buffer *Buffer)
{
    float Size = 1;
    float StartX = 0;

    for(int YIndex = 0; YIndex < 10; YIndex++) {
        for(int XIndex = 0; XIndex < 10; XIndex++) {
            float X = StartX + (XIndex * Size);
            float Y = YIndex;
            DrawRect(Buffer, V2(X, Y), V2(Size, Size), 0x00000000);
        }
    }
}

void 
SimulateGame(game_render_buffer *Buffer, game_input *Input, real32 dt)
{
    GlobalBuffer_DEBUG = Buffer;

    if(!Initialized) {
        Initialized = true;
        GlobalCurrentLevel = LEVEL_01_NORMAL;
        // GlobalCurrentLevel = LEVEL_05_PONG;
        // GlobalCurrentLevel = LEVEL_06_INVADERS;
        PowerBlockSize = V2(2, 2);

        StartGame(GlobalCurrentLevel);
    }

    real32 PaddleSpeedMultiplier = 0.17;

    // PaddleDesiredP = PixelToWorldCoord(Buffer, Input->MouseP);
    if(InvertedControlsTime > 0) { PaddleDesiredP = Paddle.P - (Input->MouseDp * PaddleSpeedMultiplier); }
    else                         { PaddleDesiredP = Paddle.P + (Input->MouseDp * PaddleSpeedMultiplier); }
    PaddleDesiredP.Y = Paddle.P.Y;

    Paddle.dP.X = PaddleDesiredP.X - Paddle.P.X;

    for(ball *Ball = Balls; Ball != Balls + ARRAY_COUNT(Balls); Ball++)
    {
        if(!(Ball->Flags & BALL_ACTIVE)) { continue; }

        Ball->DesiredP = Ball->P + (Ball->dP * dt);

        bool32 BallXOverRight  = Ball->DesiredP.X + (Ball->Size.X/2) >  Arena.P.X + (Arena.Size.X*0.5);
        bool32 BallXOverLeft   = Ball->DesiredP.X - (Ball->Size.X/2) <  Arena.P.X - (Arena.Size.X*0.5);
        bool32 BallYOverBottom = Ball->DesiredP.Y + (Ball->Size.Y/2) >  Arena.P.Y + (Arena.Size.Y*0.5);
        bool32 YOverTop        = Ball->DesiredP.Y - (Ball->Size.Y/2) <  Arena.P.Y - (Arena.Size.Y*0.5);

        if((YOverTop)) {
            ProcessBallOnDpYDown(Ball);
            if(YOverTop)        { Ball->DesiredP.Y = Arena.P.Y - Arena.Size.Y*0.5 + Ball->Size.Y*0.5; }
            if(BallYOverBottom) { Ball->DesiredP.Y = Arena.Size.Y*0.5 - Ball->Size.Y*0.5; }
            Ball->dP.Y *= -1;
        } else if((BallXOverRight) || (BallXOverLeft)) {
            ProcessBallOnDpYDown(Ball);
            if(BallXOverRight) { Ball->DesiredP.X =  Arena.Size.X*0.5 - Ball->Size.X*0.5; }
            if(BallXOverLeft)  { Ball->DesiredP.X = -Arena.Size.X*0.5 + Ball->Size.X*0.5; }
            Ball->dP.X *= -1;
        }
        if(InvincibilityTime > 0) {
            if(BallYOverBottom) { 
                Ball->DesiredP.Y =  Arena.Size.Y*0.5 - Ball->Size.Y*0.5; 
                Ball->dP.Y *= -1;
            }
        }

        for(s32 Index = 0; Index < BLOCK_COUNT; Index++)
        {
            block *Block = BlockList + Index;
            if(Block->Life) {
                if(!FirstBallMovement) {
                    static int test = 0;
                    collision_type Collision = CollisionSide(Block->P, Block->Size, V2(Ball->DesiredP.X, Ball->DesiredP.Y), Ball->Size);
                    if(COLLISION_NONE != CollisionSide(Block->P, Block->Size, V2(Ball->DesiredP.X, Ball->DesiredP.Y), Ball->Size)) {

                        GlobalScore += 1;

                        ProcessBallOnDpYDown(Ball);

                        if(! (CometTime > 0)) {
                            if(Collision == COLLISION_LEFT) { 
                                Ball->dP.X *= -1;
                            }
                            if(Collision == COLLISION_RIGHT) {
                                Ball->dP.X *= -1;
                            }
                            if(Collision == COLLISION_TOP) {
                                Ball->dP.Y *= -1;
                            }
                            if(Collision == COLLISION_BOTTOM) {
                                Ball->dP.Y *= -1;
                            }
                        }

                        if(!(StrongBlocksTime > 0)) {
                            Block->Life -= 1;
                            BlockIsDestroyed(Block);
                        }

                        break;
                    }
                }

                if(Ball->dP.Y > 0) {
                    CometTime = 0;
                }

            }
        }

        if(IsColliding(Ball->DesiredP, Ball->Size, PaddleDesiredP, Paddle.VisualSize)) {
            FirstBallMovement = false;

            Ball->dP.Y *= -1;

            float MaxMoveSpeedX = GlobalBallBaseSpeed * 2;
            float MinusOneToOneRange = -1 * ((Paddle.P.X - Ball->DesiredP.X) / ((float)Paddle.VisualSize.X / 2));
            Ball->dP.X = Clamp(-MaxMoveSpeedX, MaxMoveSpeedX * MinusOneToOneRange, MaxMoveSpeedX);

            if(GlobalNumberOfTripleShots > 0) {
                GlobalNumberOfTripleShots--;
                SpawnTripleShotsBalls(Ball->DesiredP);
            }
        } else {
            Ball->P.X = Ball->DesiredP.X;
            Ball->P.Y = Ball->DesiredP.Y;
        }

        ball_trail *Trail = Ball->Trails + Ball->NextTrail++;
        if(Ball->NextTrail >= ARRAY_COUNT(Ball->Trails)) {Ball->NextTrail = 0;}
        Trail->P =  Ball->P;
        // Trail->Life = 0.005f*ARRAY_COUNT(Ball->Trails);
        Trail->Life = 0.3f;
        Ball->TrailSpawner -= dt;
    }

    for(power *Power = PowerBlocks; 
        Power != PowerBlocks + ARRAY_COUNT(PowerBlocks);
        Power++) 
    {
        if(Power->Type == POWER_INACTIVE) { continue; }
        real32 Speed = 30;
        Power->P.Y += 1*dt*Speed;

        if(IsColliding(Power->P, PowerBlockSize, Paddle.P, Paddle.VisualSize)) {
            // Paddle.Color = 0x00000000;
            switch(Power->Type) {
                case POWER_INVINCIBILITY: {
                    InvincibilityTime += 5.0f;
                } break;

                case POWER_COMET: {
                    CometTime += 5.0f;
                } break;

                case POWER_TRIPLE_SHOT: {
                    GlobalNumberOfTripleShots++;
                } break;

                case POWER_INSTAKILL: 
                {
                    // StartGame(GlobalCurrentLevel);
                    LoseLife();
                } break;

                case POWER_STRONG_BLOCKS: 
                {
                    StrongBlocksTime += 5.0f;
                } break;

                case POWER_INVERTED_CONTROLS: 
                {
                    InvertedControlsTime += 5.0f;
                } break;

                default: {
                }break;
            }

            Power->Type = POWER_INACTIVE;
        }

    }



    // 
    // Rendering
    //

    ClearScreen(Buffer, 0x00551100);

    DrawRect(Buffer, Arena.P, Arena.Size, 0x00aa0000);

    for(s32 Index = 0; Index < BLOCK_COUNT; Index++)
    {
        block *Block = BlockList + Index;
        if(Block->Life) 
        {
            DrawRect(Buffer, Block->P, Block->Size, Block->Color);
        }
    }

    Paddle.P = PaddleDesiredP;

    {
        Paddle.VisualDp.X = (Paddle.P.X - Paddle.VisualP.X);
        Paddle.VisualP.X += (Paddle.VisualDp.X) * 0.2;

        real32 ScaleFactor = Absolute(Paddle.VisualDp.X) / 40.0f; // NOTE: 40 is just arbitrary value

        real32 PaddleScaleX = Paddle.Size.X * ScaleFactor;
        real32 PaddleScaleY = Paddle.Size.Y/2 * ScaleFactor;

        Paddle.VisualSize.X = Paddle.Size.X + PaddleScaleX;
        Paddle.VisualSize.Y = Paddle.Size.Y - PaddleScaleY;
    }

    SimulateGameMode(Buffer);
    
    SimulateLevelStateChanges(Buffer, GlobalCurrentLevel, Paddle.P);

    if(InvincibilityTime > 0) { Paddle.Color = 0x00ffffff;}
    else                      { Paddle.Color = 0x0000ff00; }

    // DrawRect(Buffer, Paddle.P, Paddle.Size, Paddle.Color);
    DrawRect(Buffer, Paddle.VisualP, Paddle.VisualSize,Paddle.Color);

    for(ball *Ball = Balls; Ball != Balls + ARRAY_COUNT(Balls); Ball++) {
        if(!(Ball->Flags & BALL_ACTIVE)) { continue; }

        for(int i = 0; i < ARRAY_COUNT(Ball->Trails); i++) {
            ball_trail *Trail = Ball->Trails + i;
            Trail->Life -= dt;
            if(Trail->Life <= 0) { continue; }
            DrawRectAlpha(Buffer, Trail->P, Ball->Size, Ball->Color, Trail->Life); 
        }

        DrawRect(Buffer, V2(Ball->P.X, Ball->P.Y), Ball->Size, Ball->Color);
    }

    if(Balls[0].P.Y > (Arena.Size.Y / 2)) {
        LoseLife();
    }

    if(AdvanceGameMode) {
        GlobalCurrentLevel = (level)(GlobalCurrentLevel + 1);
        StartGame(GlobalCurrentLevel);
    }

    if(InvincibilityTime > 0) { InvincibilityTime -= dt; }
    if(CometTime > 0) { CometTime -= dt; }
    if(StrongBlocksTime > 0) { StrongBlocksTime -= dt; }
    if(InvertedControlsTime > 0) { InvertedControlsTime -= dt; }

    for(int i = 0; i < PlayerLifeCount; i++) {
        // DrawRect();
        real32 Size = 2;
        real32 Offset = 0.7;
        real32 StartX = Arena.P.X - (Arena.Size.X/2) - 3;
        real32 X = StartX + i*Size + i*Offset;
        real32 Y = Arena.P.Y - (Arena.Size.Y/2) - 2.5;
        DrawRect(Buffer, V2(X, Y), V2(Size, Size), 0x0000FF00);
    }


    DrawNumber(Buffer, GlobalScore, V2(-Arena.Size.X/2 + 10, -Arena.Size.Y/2 + 1.5), 1.5, 0xffffffff);

    // ConsolePrintInt((int)Paddle.P.X);
    // static int OldPaddlePX = 0;
    // if(OldPaddlePX != (int)Paddle.P.X) {
    //     printf("%d %d\n", OldPaddlePX, (int)Paddle.P.X);
    //     ConsolePrintInt((int)Paddle.P.X);
    // }
    // OldPaddlePX = (int)Paddle.P.X;
    DrawConsoleMessages(Buffer, &Arena, dt);


#if DEVELOPMENT
    if(IS_CHANGED(Input, BUTTON_LEFT)) { StartGame((level)(GlobalCurrentLevel - 1)); }
    if(IS_CHANGED(Input, BUTTON_RIGHT)) { StartGame((level)(GlobalCurrentLevel + 1)); }

    // if(IS_CHANGED(Input, BUTTON_LEFT)) { 
    //     test--;
    // }
    // if(IS_CHANGED(Input, BUTTON_RIGHT)) { 
    //     test++;
    // }
    
    if(IS_PRESSED(Input, BUTTON_DOWN)) { 
        static int test = 0;
        printf("test: %d\n", test);
        ConsolePrintInt(test++);
    }

    if(IS_CHANGED(Input, BUTTON_UP)) { 
        if(InvincibilityTime > 0) { InvincibilityTime = 0.0f; }
        else { InvincibilityTime = 20.0f; }
    }
    // if(IS_PRESSED(Input, BUTTON_DOWN))  { Paddle.P.Y += Speed; }
#endif

}
