#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <ppu-types.h>
#include <sysutil/sysutil.h>
#include <sysutil/video.h>
#include <rsx/rsx.h>
#include <io/pad.h>
#define CB_SIZE (0x100000)
#define HOST_SIZE (32*1024*1024)
static gcmContextData *ctx; static u32 W,H,*fb[2],fboff[2],*back; static int cur=0,first_flip=1; static volatile int running=1;
static void sysutil_cb(u64 s,u64 p,void*u){(void)p;(void)u; if(s==SYSUTIL_EXIT_GAME) running=0;}
static void init_screen(void){ void *host=memalign(1024*1024,HOST_SIZE); rsxInit(&ctx,CB_SIZE,HOST_SIZE,host); videoState st; videoGetState(VIDEO_PRIMARY,0,&st); u32 resid=st.displayMode.resolution; if(videoGetResolutionAvailability(VIDEO_PRIMARY,VIDEO_RESOLUTION_720,VIDEO_ASPECT_AUTO,0)) resid=VIDEO_RESOLUTION_720; videoResolution res; videoGetResolution(resid,&res); W=res.width; H=res.height; videoConfiguration vc; memset(&vc,0,sizeof(vc)); vc.resolution=resid; vc.format=VIDEO_BUFFER_FORMAT_XRGB; vc.aspect=VIDEO_ASPECT_AUTO; vc.pitch=W*4; videoConfigure(VIDEO_PRIMARY,&vc,NULL,0); videoGetState(VIDEO_PRIMARY,0,&st); gcmSetFlipMode(GCM_FLIP_VSYNC); for(int i=0;i<2;i++){fb[i]=(u32*)rsxMemalign(64,W*H*4); gcmAddressToOffset(fb[i],&fboff[i]); gcmSetDisplayBuffer(i,fboff[i],W*4,W,H);} back=(u32*)memalign(64,W*H*4); memset(back,0,W*H*4); gcmResetFlipStatus(); }
static void wait_flip(void){ while(gcmGetFlipStatus()!=0) usleep(200); gcmResetFlipStatus(); }
static void present(void){ if(!first_flip) wait_flip(); else gcmResetFlipStatus(); memcpy(fb[cur],back,W*H*4); gcmSetFlip(ctx,cur); rsxFlushBuffer(ctx); gcmSetWaitFlip(ctx); cur^=1; first_flip=0; }
static void fill_rect(int x,int y,int w,int h,u32 c){ int x0=x<0?0:x,y0=y<0?0:y,x1=x+w,y1=y+h; if(x1>(int)W)x1=W; if(y1>(int)H)y1=H; for(int yy=y0;yy<y1;yy++){u32*p=back+yy*W; for(int xx=x0;xx<x1;xx++)p[xx]=c;} }
static void fill_circle(int cx,int cy,int r,u32 c){ for(int y=-r;y<=r;y++) for(int x=-r;x<=r;x++) if(x*x+y*y<=r*r) fill_rect(cx+x,cy+y,1,1,c); }
typedef struct { char c; const char *p; } Glyph;
static const Glyph glyphs[]={ {'0',"###" "#.#" "#.#" "#.#" "###"},{'1',".#." "##." ".#." ".#." "###"},{'2',"###" "..#" "###" "#.." "###"},{'3',"###" "..#" "###" "..#" "###"},{'4',"#.#" "#.#" "###" "..#" "..#"},{'5',"###" "#.." "###" "..#" "###"},{'6',"###" "#.." "###" "#.#" "###"},{'7',"###" "..#" "..#" "..#" "..#"},{'8',"###" "#.#" "###" "#.#" "###"},{'9',"###" "#.#" "###" "..#" "###"},{'A',".#." "#.#" "###" "#.#" "#.#"},{'B',"##." "#.#" "##." "#.#" "##."},{'C',"###" "#.." "#.." "#.." "###"},{'D',"##." "#.#" "#.#" "#.#" "##."},{'E',"###" "#.." "###" "#.." "###"},{'F',"###" "#.." "###" "#.." "#.."},{'G',"###" "#.." "#.#" "#.#" "###"},{'I',"###" ".#." ".#." ".#." "###"},{'L',"#.." "#.." "#.." "#.." "###"},{'M',"#.#" "###" "###" "#.#" "#.#"},{'N',"#.#" "###" "###" "#.#" "#.#"},{'O',"###" "#.#" "#.#" "#.#" "###"},{'P',"###" "#.#" "###" "#.." "#.."},{'R',"##." "#.#" "##." "#.#" "#.#"},{'S',"###" "#.." "###" "..#" "###"},{'T',"###" ".#." ".#." ".#." ".#."},{'U',"#.#" "#.#" "#.#" "#.#" "###"},{'V',"#.#" "#.#" "#.#" "#.#" ".#."},{'X',"#.#" "#.#" ".#." "#.#" "#.#"},{'Y',"#.#" "#.#" ".#." ".#." ".#."},{':',"..." ".#." "..." ".#." "..."},{'.',"..." "..." "..." "..." ".#."},{'-',"..." "..." "###" "..." "..."},{' ',"..." "..." "..." "..." "..."} };
static const char *find_glyph(char c){ for(unsigned i=0;i<sizeof(glyphs)/sizeof(glyphs[0]);i++) if(glyphs[i].c==c) return glyphs[i].p; return NULL; }
static void draw_text(const char *s,int x,int y,int sc,u32 c){ for(;*s;s++){ const char *g=find_glyph(*s); if(g) for(int r=0;r<5;r++) for(int k=0;k<3;k++) if(g[r*3+k]=='#') fill_rect(x+k*sc,y+r*sc,sc,sc,c); x+=4*sc; } }
static void draw_text_center(const char *s,int y,int sc,u32 c){ int w=strlen(s)*4*sc; draw_text(s,(W-w)/2,y,sc,c); }
typedef struct { float x,y,vx,vy; int life; u32 c; } Part;
static Part parts[40];
static void spawn_part(float x,float y,u32 c){ for(int i=0;i<40;i++) if(parts[i].life<=0){ parts[i].x=x; parts[i].y=y; parts[i].vx=-2+rand()%5; parts[i].vy=-3+rand()%3; parts[i].life=20+rand()%15; parts[i].c=c; break; } }
int main(int argc,char**argv){ (void)argc;(void)argv; init_screen(); ioPadInit(7); sysUtilRegisterCallback(0,sysutil_cb,NULL);
float birdY=320, vel=0, angle=0; int score=0, best=0, state=0, frame=0, shake=0;
int pipeX[3]={950,1350,1750}; int pipeGapY[3]={320,270,360}; int passed[3]={0,0,0};
int x_prev=0; float groundOff=0, cloudX[3]={100,500,900}; float wing=0; int scorePop=0;
while(running){ sysUtilCheckCallback(); frame++;
padInfo info; padData d; int xb=0; ioPadGetInfo(&info); if(info.status[0]){ ioPadGetData(0,&d); xb=d.BTN_CROSS; }
if(xb && !x_prev){
  if(state==0){ state=1; vel=-9.5f; wing=0; spawn_part(300,birdY+20,0x00FFFFFF); }
  else if(state==1){ vel=-9.5f; wing=0; spawn_part(300,birdY+20,0x00FFFFFF); }
  else if(state==2){ state=0; score=0; birdY=320; vel=0; angle=0; pipeX[0]=950; pipeX[1]=1350; pipeX[2]=1750; pipeGapY[0]=320; pipeGapY[1]=270; pipeGapY[2]=360; passed[0]=passed[1]=passed[2]=0; }
}
x_prev=xb; wing+=0.35f;
if(state==0){ birdY=320+sinf(frame*0.08f)*12; angle=sinf(frame*0.08f)*0.15f; }
if(state==1){
  vel+=0.55f; if(vel>13) vel=13; birdY+=vel;
  angle = vel*0.06f; if(angle>1.2f) angle=1.2f; if(angle<-0.5f) angle=-0.5f;
  groundOff+=4; for(int i=0;i<3;i++) cloudX[i]-=0.8f; if(cloudX[0]<-200) cloudX[0]=1300; if(cloudX[1]<-200) cloudX[1]=1300; if(cloudX[2]<-200) cloudX[2]=1300;
  for(int i=0;i<3;i++){ pipeX[i]-=4; if(pipeX[i]<-90){ pipeX[i]=1250; pipeGapY[i]=220+rand()%280; passed[i]=0; } if(!passed[i] && pipeX[i]<300){ passed[i]=1; score++; scorePop=20; if(score>best) best=score; for(int p=0;p<5;p++) spawn_part(350,300,0x00FFD400); } int bx=300, by=(int)birdY; int px=pipeX[i], gy=pipeGapY[i]; if(bx+38>px && bx<px+78){ if(by<gy-85 || by+38>gy+85){ state=2; shake=15; } } } if(birdY<0) birdY=0; if(birdY>(int)H-110){ birdY=H-110; state=2; shake=15; }
}
if(state==2){ if(birdY<(int)H-110){ vel+=0.7f; birdY+=vel; angle+=0.08f; if(angle>1.4f) angle=1.4f; } if(shake>0) shake--; }
if(scorePop>0) scorePop--;
for(int i=0;i<40;i++) if(parts[i].life>0){ parts[i].x+=parts[i].vx; parts[i].y+=parts[i].vy; parts[i].vy+=0.2f; parts[i].life--; }
int shx=0, shy=0; if(shake>0){ shx=rand()%6-3; shy=rand()%6-3; }
// DESENHO 3D
for(int y=0;y<(int)H;y++){ int r=70+(y*60)/720; int g=170+(y*30)/720; int b=255-(y*20)/720; u32 c=(r<<16)|(g<<8)|b; fill_rect(0,y+shy,W,1,c); }
fill_circle(1050+shx,110+shy,45,0x00FFE600); fill_circle(1050+shx,110+shy,38,0x00FFF200);
for(int i=0;i<3;i++){ int cx=(int)cloudX[i]+shx, cy=90+i*60; fill_circle(cx,cy,28,0x00FFFFFF); fill_circle(cx+30,cy+8,22,0x00FFFFFF); fill_circle(cx+60,cy,24,0x00FFFFFF); fill_rect(cx-5,cy+5,80,18,0x00FFFFFF); }
fill_circle(150+shx,650+shy,220,0x0090D090); fill_circle(600+shx,680+shy,260,0x0070C070); fill_circle(1100+shx,660+shy,220,0x0090D090);
// Canos 3D com brilho e sombra
for(int i=0;i<3;i++){ int px=pipeX[i]+shx, gy=pipeGapY[i]+shy;
  fill_rect(px,0,78,gy-85,0x0030A030); fill_rect(px+6,0,12,gy-85,0x0070E070); fill_rect(px+58,0,12,gy-85,0x00104010); fill_rect(px-6,gy-115,90,30,0x00208020); fill_rect(px,gy-110,78,6,0x0090FF90); fill_rect(px+90,0,8,gy-85,0x00000000);
  fill_rect(px,gy+85,78,(int)H-(gy+85),0x0030A030); fill_rect(px+6,gy+85,12,(int)H-(gy+85),0x0070E070); fill_rect(px+58,gy+85,12,(int)H-(gy+85),0x00104010); fill_rect(px-6,gy+85,90,30,0x00208020); fill_rect(px,gy+108,78,6,0x0090FF90); fill_rect(px+90,gy+85,8,(int)H-(gy+85),0x00000000);
}
fill_rect(0,(int)H-70+shy,W,70,0x00D8B45A); fill_rect(0,(int)H-70+shy,W,8,0x0080C020);
for(int x=-40;x<(int)W+40;x+=80){ int gx=x-((int)groundOff%80); fill_rect(gx,(int)H-50+shy,40,50,0x00C0A040); }
for(int i=0;i<40;i++) if(parts[i].life>0) fill_circle((int)parts[i].x+shx,(int)parts[i].y+shy,3,parts[i].c);
// PASSARINHO 3D
int bx=300+shx, by=(int)birdY+shy;
int flap = (int)(sinf(wing)*8);
fill_circle(bx+32,by+22,30,0x00000000); // sombra 3D
fill_circle(bx+30,by+18,28,0x00FFE600);
fill_circle(bx+20,by+10,22,0x00FFF200);
fill_rect(bx-12,by+8,18,10,0x00FF8000);
fill_circle(bx+38,by+6+flap,14,0x00FF9D00);
fill_circle(bx+38,by+6+flap,8,0x00FFC800);
fill_circle(bx+45,by+2,10,0x00FFFFFF);
fill_circle(bx+48,by+2,5,0x00000000);
fill_circle(bx+48,by+1,2,0x00FFFFFF);
fill_rect(bx+52,by+12,18,8,0x00FF4D00);
fill_rect(bx+52,by+20,14,6,0x00E60000);
if(state!=0){ char buf[16]; snprintf(buf,sizeof(buf),"%d",score); int sc=10+scorePop/2; draw_text_center(buf,60,sc,0x00000000); draw_text_center(buf,58,sc,0x00FFFFFF); }
if(state==0){ draw_text_center("FLAPPY 3D",100,12,0x00000000); draw_text_center("FLAPPY 3D",98,12,0x00FFFFFF); draw_text_center("3D",150,10,0x00FFE600); fill_rect(340,220,600,70,0x00FFD400); fill_rect(340,220,600,8,0x00FF9D00); draw_text_center("APERTE X P/ VOAR",240,7,0x00000000); draw_text_center("TENTE FAZER 10 PONTOS",330,6,0x00FFFFFF); }
if(state==2){ fill_rect(290,180,700,320,0x00C89030); fill_rect(300,190,680,300,0x00FFE6A0); draw_text_center("GAME OVER",210,10,0x00FF0000); char buf[64]; snprintf(buf,sizeof(buf),"SCORE %d  BEST %d",score,best); draw_text_center(buf,300,6,0x00000000); if(score>=30) draw_text_center("MEDALHA OURO!",360,7,0x00FFAA00); else if(score>=20) draw_text_center("MEDALHA PRATA!",360,7,0x00AAAAAA); else if(score>=10) draw_text_center("MEDALHA BRONZE!",360,7,0x00AA5500); else draw_text_center("TENTE DE NOVO!",360,7,0x00000000); draw_text_center("APERTE X",430,7,0x00000000); }
present(); usleep(16000); }
return 0; }