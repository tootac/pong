
typedef enum {
    PROFITEM_INPUT,
    PROFITEM_GAME,
    PROFITEM_FLIP,

    PROFITEM_DRAW_RECT,
    PROFITEM_DRAW_RECT_ALPHA,

    PROFITEM__COUNT,
} profiling_item;


#if PROFILING

typedef struct {
    u64 HitCount;
    double BeginTime;
    double Time;
} perf_data;


void ProfilerStart(profiling_item Item);
void ProfilerEnd(profiling_item Item);


#else

#define ProfilerClearData(...) 
#define ProfilerEnd(...) 
#define ProfilerStart(...) 
#define RenderProfiler(...) 

#endif
