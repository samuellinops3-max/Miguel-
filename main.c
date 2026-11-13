// Geometry Dash PS3 - nativo para PS3 (PSL1GHT)
// Controles: X = PULAR, Quadrado = Pular tbm, Start = Restart
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <malloc.h>
#include <ppu-types.h>
#include <sysutil/sysutil.h>
#include <sysutil/video.h>
#include <rsx/rsx.h>
#include <io/pad.h>

#define CB_SIZE   (0x100000)
#define HOST_SIZE (32 * 1024 * 1024)
#define MAXOBS 60
#define PI 3.14159265f

/* ---------- video - IGUAL DO COBRA ---------- */
static gcmContextData *ctx;
static u32 W, H;
static u32 *fb[2];
static u32 fboff[2];
static u32 *back;
static int cur = 0, first_flip = 1;
static volatile int running = 1;

static void sysutil_cb(u64 status, u64 param, void *usrdata)
{
    (void)param; (void)usrdata;
    if (status == SYSUTIL_EXIT_GAME) running = 0;
}

static void init_screen(void)
{
    void *host = memalign(1024 * 1024, HOST_SIZE);
    rsxInit(&ctx, CB_SIZE, HOST_SIZE, host);
    videoState st;
    videoGetState(VIDEO_PRIMARY, 0, &st);
    u32 resid = st.displayMode.resolution;
    if (videoGetResolutionAvailability(VIDEO_PRIMARY, VIDEO_RESOLUTION_720, VIDEO_ASPECT_AUTO, 0))
        resid = VIDEO_RESOLUTION_720;
    videoResolution res;
    videoGetResolution(resid, &res);
    W = res.width; H = res.height;
    videoConfiguration vc;
    memset(&vc, 0, sizeof(vc));
    vc.resolution = resid;
    vc.format = VIDEO_BUFFER_FORMAT_XRGB;
    vc.aspect = VIDEO_ASPECT_AUTO;
    vc.pitch = W * 4;
    videoConfigure(VIDEO_PRIMARY, &vc, NULL, 0);
    videoGetState(VIDEO_PRIMARY, 0, &st);
    gcmSetFlipMode(GCM_FLIP_VSYNC);
    for (int i = 0; i < 2; i++) {
        fb[i] = (u32 *)rsxMemalign(64, W * H * 4);
        gcmAddressToOffset(fb[i], &fboff[i]);
        gcmSetDisplayBuffer(i, fboff[i], W * 4, W, H);
    }
    back = (u32 *)memalign(64, W * H * 4);
    memset(back, 0, W * H * 4);
    gcmResetFlipStatus();
}
static void wait_flip(void){ while(gcmGetFlipStatus()!=0) usleep(200); gcmResetFlipStatus(); }
static void present(void){
    if(!first_flip) wait_flip(); else gcmResetFlipStatus();
    memcpy(fb[cur], back, W*H*4);
    gcmSetFlip(ctx, cur); rsxFlushBuffer(ctx); gcmSetWaitFlip(ctx);
    cur ^= 1; first_flip = 0;
}

/* ---------- desenho ---------- */
static void fill_rect(int x,int y,int w,int h,u32 c){
    int x0=x<0?0:x, y0=y<0?0:y;
    int x1=x+w, y1=y+h;
    if(x1>(int)W) x1=W; if(y1>(int)H) y1=H;
    for(int yy=y0;yy<y1;yy++){ u32 *p=back+yy*W; for(int xx=x0;xx<x1;xx++) p[xx]=c; }
}
static void triangle(int x,int y,int w,int h,u32 c){
    for(int dy=0;dy<h;dy++){
        int yy=y+dy;
        if(yy<0||yy>=(int)H) continue;
        float t=(float)dy/(float)h;
        int hw=(int)(w*t/2);
        int cx=x+w/2;
        int x0=cx-hw, x1=cx+hw;
        if(x0<0) x0=0; if(x1>=(int)W) x1=W-1;
        u32 *p=back+yy*W;
        for(int xx=x0;xx<=x1;xx++) p[xx]=c;
    }
}

/* fonte 3x5 */
typedef struct { char c; const char *p; } Glyph;
static const Glyph glyphs[] = {
    {'0',"###" "#.#" "#.#" "#.#" "###"}, {'1',".#." "##." ".#." ".#." "###"},
    {'2',"###" "..#" "###" "#.." "###"}, {'3',"###" "..#" "###" "..#" "###"},
    {'4',"#.#" "#.#" "###" "..#" "..#"}, {'5',"###" "#.." "###" "..#" "###"},
    {'6',"###" "#.." "###" "#.#" "###"}, {'7',"###" "..#" "..#" "..#" "..#"},
    {'8',"###" "#.#" "###" "#.#" "###"}, {'9',"###" "#.#" "###" "..#" "###"},
    {'A',".#." "#.#" "###" "#.#" "#.#"}, {'B',"##." "#.#" "##." "#.#" "##."},
    {'C',"###" "#.." "#.." "#.." "###"}, {'D',"##." "#.#" "#.#" "#.#" "##."},
    {'E',"###" "#.." "###" "#.." "###"}, {'G',"###" "#.." "#.#" "#.#" "###"},
    {'M',"#.#" "###" "###" "#.#" "#.#"}, {'O',"###" "#.#" "#.#" "#.#" "###"},
    {'P',"###" "#.#" "###" "#.." "#.."}, {'R',"##." "#.#" "##." "#.#" "#.#"},
    {'S',"###" "#.." "###" "..#" "###"}, {'T',"###" ".#." ".#." ".#." ".#."},
    {'U',"#.#" "#.#" "#.#" "#.#" "###"}, {'V',"#.#" "#.#" "#.#" "#.#" ".#."},
    {'X',"#.#" "#.#" ".#." "#.#" "#.#"}, {'Y',"#.#" "#.#" ".#." ".#." ".#."},
    {'H',"#.#" "#.#" "###" "#.#" "#.#"}, {'L',"#.." "#.." "#.." "#.." "###"},
    {'N',"#.#" "###" "###" "#.#" "#.#"}, {'I',"###" ".#." ".#." ".#." "###"},
    {'F',"###" "#.." "###" "#.." "#.."}, {'J',"###" "..#" "..#" "#.#" "###"},
    {'K',"#.#" "#.#" "##." "#.#" "#.#"}, {'W',"#.#" "#.#" "###" "###" "#.#"},
    {'Q',"###" "#.#" "#.#" "###" "..#"}, {'Z',"###" "..#" ".#." "#.." "###"},
    {'%',"##." ".#." ".#." ".#." "##."}, {'!',"#.#" "#.#" "#.#" "..." "#.#"},
};
static const char *find_glyph(char c){ for(unsigned i=0;i<sizeof(glyphs)/sizeof(glyphs[0]);i++) if(glyphs[i].c==c) return glyphs[i].p; return NULL; }
static int text_width(const char *s,int sc){ return (int)strlen(s)*4*sc - sc; }
static void draw_text(const char *s,int x,int y,int sc,u32 c){
    for(;*s;s++){ const char *g=find_glyph(*s); if(g) for(int r=0;r<5;r++) for(int k=0;k<3;k++) if(g[r*3+k]=='#') fill_rect(x+k*sc,y+r*sc,sc,sc,c); x+=4*sc; }
}

/* ---------- GEOMETRY DASH ---------- */
typedef struct { float x; int tipo; int w, h; } Obs;
static Obs obs[MAXOBS];
static int nobs = 0;
static float player_x, player_y, player_vy;
static float rot = 0;
static int on_ground = 1, game_over = 0, dead_t = 0, score = 0, best = 0, jumps = 0, level = 1;
static float distancia = 0, fim = 2500;
static float speed = 6.0f;
static int pad_jump = 0, pad_start = 0, prev_jump = 0, prev_start = 0;
static float ground_y;

static float frand(void){ return (float)rand()/(float)RAND_MAX; }

static void gen_level(void){
    nobs = 0;
    float x = 500;
    float end = fim;
    while(x < end && nobs < MAXOBS){
        int r = rand() % 100;
        if(r < 40){
            obs[nobs].x = x; obs[nobs].tipo = 0; obs[nobs].w = 30; obs[nobs].h = 30; nobs++;
            x += 260 + frand()*120;
        } else if(r < 60){
            obs[nobs].x = x; obs[nobs].tipo = 0; obs[nobs].w = 30; obs[nobs].h = 30; nobs++;
            obs[nobs].x = x+32; obs[nobs].tipo = 0; obs[nobs].w = 30; obs[nobs].h = 30; nobs++;
            x += 380 + frand()*100;
        } else if(r < 80){
            obs[nobs].x = x; obs[nobs].tipo = 1; obs[nobs].w = 55; obs[nobs].h = 50; nobs++;
            x += 320 + frand()*100;
        } else {
            obs[nobs].x = x; obs[nobs].tipo = 1; obs[nobs].w = 55; obs[nobs].h = 50; nobs++;
            obs[nobs].x = x+70; obs[nobs].tipo = 0; obs[nobs].w = 30; obs[nobs].h = 30; nobs++;
            x += 420 + frand()*100;
        }
    }
}

static void init_game(void){
    ground_y = H * 0.78f;
    player_x = W * 0.18f;
    player_y = ground_y - 36;
    player_vy = 0; rot = 0; on_ground = 1;
    game_over = 0; dead_t = 0; score = 0; jumps = 0; distancia = 0;
    if(level==1){ speed=6.0f; fim=2500; }
    else if(level==2){ speed=7.5f; fim=3500; }
    else { speed=9.0f; fim=4500; }
    gen_level();
}

static void read_pad(void){
    padInfo info; padData d;
    pad_jump = pad_start = 0;
    ioPadGetInfo(&info);
    if(info.status[0]){
        ioPadGetData(0, &d);
        pad_jump = d.BTN_CROSS || d.BTN_SQUARE || d.BTN_CIRCLE;
        pad_start = d.BTN_START;
    }
}

static void update_game(void){
    if(game_over){
        dead_t++;
        if(dead_t > 25 && pad_jump && !prev_jump) init_game();
        if(pad_start && !prev_start){ level=1; init_game(); }
        prev_jump = pad_jump; prev_start = pad_start;
        return;
    }
    if(pad_jump && !prev_jump && on_ground){
        player_vy = -15.0f;
        on_ground = 0; jumps++;
    }
    prev_jump = pad_jump; prev_start = pad_start;

    player_vy += 0.85f;
    player_y += player_vy;
    rot += speed * 0.6f;
    if(player_y >= ground_y - 36){
        player_y = ground_y - 36; player_vy = 0; on_ground = 1; rot = 0;
    }

    distancia += speed;
    score = (int)(distancia / fim * 100);
    if(score > 100) score = 100;
    if(score > best) best = score;

    for(int i=0;i<nobs;i++){
        float ox = obs[i].x - distancia + 500;
        float oy, ow = obs[i].w, oh = obs[i].h;
        if(obs[i].tipo==0) oy = ground_y - oh;
        else oy = ground_y - oh;
        float px = player_x, py = player_y;
        if(px+28 > ox+5 && px+8 < ox+ow-5 && py+30 > oy+6 && py+6 < oy+oh){
            if(obs[i].tipo==1 && player_vy > 0 && py+30 < oy+18){
                player_y = oy - 36; player_vy = 0; on_ground = 1; rot = 0;
            } else {
                game_over = 1; dead_t = 0;
                return;
            }
        }
    }
    if(distancia >= fim){
        if(level < 3){ level++; init_game(); }
        else { game_over = 1; dead_t = 0; }
    }
}

static void render(void){
    float scf = (float)W / 1280.0f;
    int ui = (int)(4*scf+0.5f); if(ui<3) ui=3;
    u32 cor = 0x0000FF66;
    if(level==2) cor = 0x00FFAA00;
    if(level==3) cor = 0x00FF0055;
    u32 fundo = 0x000A0A2E;
    if(level==2) fundo = 0x002E0A0A;
    if(level==3) fundo = 0x001A001A;
    for(u32 i=0;i<W*H;i++) back[i]=fundo;
    int off = (int)distancia % 40;
    for(int x=-off; x<(int)W; x+=40) fill_rect(x,0,1,H,0x00111133);
    for(int y=0;y<(int)H;y+=40) fill_rect(0,y,W,1,0x00111133);
    fill_rect(0, (int)ground_y, W, (int)H - (int)ground_y, 0x00111111);
    fill_rect(0, (int)ground_y, W, 4, cor);
    for(int i=0;i<nobs;i++){
        float ox = obs[i].x - distancia + 500;
        if(ox < -100 || ox > W+100) continue;
        if(obs[i].tipo==0){
            triangle((int)ox, (int)(ground_y - obs[i].h), obs[i].w, obs[i].h, 0x00FFFFFF);
            triangle((int)ox+5, (int)(ground_y - obs[i].h)+6, obs[i].w-10, obs[i].h-6, cor);
        } else {
            fill_rect((int)ox, (int)(ground_y - obs[i].h), obs[i].w, obs[i].h, 0x00222222);
            fill_rect((int)ox, (int)(ground_y - obs[i].h), obs[i].w, 3, cor);
            fill_rect((int)ox, (int)(ground_y - 3), obs[i].w, 3, cor);
            fill_rect((int)ox, (int)(ground_y - obs[i].h), 3, obs[i].h, cor);
            fill_rect((int)ox+obs[i].w-3, (int)(ground_y - obs[i].h), 3, obs[i].h, cor);
        }
    }
    int px = (int)player_x, py = (int)player_y;
    int s = 36;
    fill_rect(px+4, (int)ground_y-4, s, 4, 0x00000000);
    fill_rect(px, py, s, s, cor);
    fill_rect(px, py, s, 3, 0x00FFFFFF);
    fill_rect(px, py+s-3, s, 3, 0x00FFFFFF);
    fill_rect(px, py, 3, s, 0x00FFFFFF);
    fill_rect(px+s-3, py, 3, s, 0x00FFFFFF);
    fill_rect(px+10, py+10, 16, 16, 0x00FFFFFF);
    fill_rect(px+14, py+14, 8, 8, 0x00000000);
    char buf[32];
    draw_text("GEOMETRY DASH", 20, 20, ui, 0x00FFFFFF);
    sprintf(buf, "%d%%", score);
    draw_text(buf, 20, 20+7*ui, ui+2, 0x00FFD23F);
    sprintf(buf, "NIVEL %d", level);
    draw_text(buf, (int)W - text_width(buf, ui) - 20, 20, ui, 0x00FFFFFF);
    sprintf(buf, "PULOS %d", jumps);
    draw_text(buf, (int)W - text_width(buf, ui) - 20, 20+7*ui, ui, 0x00AAAAAA);
    fill_rect(20, 20+14*ui, (int)W-40, 8, 0x00333333);
    fill_rect(20, 20+14*ui, (int)((W-40)*score/100), 8, cor);
    draw_text("X PULAR", 20, (int)H-28, ui, 0x00809BBF);
    if(game_over){
        int bw = text_width("MORREU", ui*2)+80, bh=16*ui+60;
        fill_rect((int)W/2-bw/2, (int)H/2-bh/2, bw, bh, 0x00000000);
        if(score>=100 && level==3){
            draw_text("ZEROU", (int)W/2-text_width("ZEROU", ui*2)/2, (int)H/2-bh/2+24, ui*2, 0x0000FF66);
        } else {
            draw_text("MORREU", (int)W/2-text_width("MORREU", ui*2)/2, (int)H/2-bh/2+24, ui*2, 0x00FF4D4D);
        }
        sprintf(buf, "CHEGOU %d%%", score);
        draw_text(buf, (int)W/2-text_width(buf, ui)/2, (int)H/2+ui*2, ui, 0x00FFFFFF);
        if(dead_t>25) draw_text("PRESS X", (int)W/2-text_width("PRESS X", ui)/2, (int)H/2+ui*5, ui, 0x00FFFFFF);
    }
}

int main(int argc, char **argv){
    (void)argc; (void)argv;
    init_screen();
    ioPadInit(7);
    sysUtilRegisterCallback(0, sysutil_cb, NULL);
    srand(20261112);
    init_game();
    while(running){
        sysUtilCheckCallback();
        read_pad();
        update_game();
        render();
        present();
    }
    return 0;
}