#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysutil/video.h>
#include <rsx/rsx.h>
#include <io/pad.h>

// ========== LITTLE NIGHTMARES: SOMBRA - PS3 INDIE ==========
// 720p 60fps, fisica, stealth, IA do monstro, puzzles

#define WIDTH 1280
#define HEIGHT 720
#define GRAVIDADE 0.8f
#define CHAO_Y 600

typedef struct {
    float x, y, vx, vy;
    int w, h;
    int noChao;
    int escondido;
    int temLanterna;
    int chaves;
    int vida;
    int correndo;
} Player;

typedef struct {
    float x, y, vx;
    int w, h;
    int estado; // 0 patrulha, 1 perseguindo, 2 procurando
    int direcao;
    int visao;
    float timer;
} Monstro;

typedef struct {
    float x, y;
    int pegou;
    int w, h;
} Chave;

typedef struct {
    float x, y, w, h;
    int tipo; // 0 plataforma, 1 esconderijo, 2 porta, 3 espinho
} Objeto;

Player six;
Monstro chefe;
Chave chaves[3];
Objeto mapa[12];
int fase = 1;
int gameOver = 0;
int venceu = 0;
int tempo = 0;
int barulho = 0;

void init_fase(int f) {
    six.x = 50; six.y = 500; six.vx = 0; six.vy = 0;
    six.w = 32; six.h = 54; six.vida = 3;
    six.chaves = 0; six.escondido = 0; six.temLanterna = 1;

    chefe.x = 900; chefe.y = 500; chefe.w = 70; chefe.h = 110;
    chefe.estado = 0; chefe.direcao = -1; chefe.visao = 250; chefe.timer = 0;

    // Chaves espalhadas
    chaves[0].x = 400; chaves[0].y = 450; chaves[0].pegou = 0;
    chaves[1].x = 700; chaves[1].y = 300; chaves[1].pegou = 0;
    chaves[2].x = 1100; chaves[2].y = 500; chaves[2].pegou = 0;
    for(int i=0;i<3;i++){ chaves[i].w=20; chaves[i].h=20; }

    // Mapa: plataformas e esconderijos
    memset(mapa,0,sizeof(mapa));
    mapa[0]=(Objeto){0, CHAO_Y, 1280, 120, 0};
    mapa[1]=(Objeto){200, 480, 180, 20, 0};
    mapa[2]=(Objeto){500, 380, 180, 20, 0};
    mapa[3]=(Objeto){800, 480, 180, 20, 0};
    mapa[4]=(Objeto){300, 550, 80, 80, 1}; // mesa pra esconder
    mapa[5]=(Objeto){950, 500, 60, 100, 1}; // armario
    mapa[6]=(Objeto){1180, 450, 60, 150, 2}; // porta final
    mapa[7]=(Objeto){600, 580, 60, 20, 3}; // armadilha
}

int colide(float x1,float y1,int w1,int h1,float x2,float y2,int w2,int h2){
    return (x1 < x2+w2 && x1+w1 > x2 && y1 < y2+h2 && y1+h1 > y2);
}

void update_player(padInfo *pad, padData *data){
    six.vx = 0;
    six.correndo = 0;

    if(data->button[PAD_BUTTON_LEFT]){ six.vx = -4; }
    if(data->button[PAD_BUTTON_RIGHT]){ six.vx = 4; six.correndo = 1; }
    if(data->button[PAD_BUTTON_CROSS] && six.noChao){
        six.vy = -15; six.noChao = 0;
    }
    // Esconder com Quadrado perto de esconderijo
    if(data->button[PAD_BUTTON_SQUARE]){
        for(int i=0;i<12;i++) if(mapa[i].tipo==1 && colide(six.x,six.y,six.w,six.h,mapa[i].x,mapa[i].y,mapa[i].w,mapa[i].h)){
            six.escondido = 1;
        }
    } else {
        six.escondido = 0;
    }

    // Fisica
    six.vy += GRAVIDADE;
    six.x += six.vx;
    six.y += six.vy;
    six.noChao = 0;

    // Colisao com chao e plataformas
    for(int i=0;i<12;i++){
        if(mapa[i].tipo==0 && colide(six.x,six.y,six.w,six.h,mapa[i].x,mapa[i].y,mapa[i].w,mapa[i].h)){
            if(six.vy > 0 && six.y + six.h - six.vy <= mapa[i].y + 5){
                six.y = mapa[i].y - six.h;
                six.vy = 0; six.noChao = 1;
            }
        }
    }
    if(six.x < 0) six.x = 0;
    if(six.x > WIDTH - six.w) six.x = WIDTH - six.w;

    // Pegar chaves
    for(int i=0;i<3;i++){
        if(!chaves[i].pegou && colide(six.x,six.y,six.w,six.h,chaves[i].x,chaves[i].y,chaves[i].w,chaves[i].h)){
            chaves[i].pegou = 1; six.chaves++;
        }
    }

    // Porta final
    if(six.chaves >= 3 && colide(six.x,six.y,six.w,six.h,mapa[6].x,mapa[6].y,mapa[6].w,mapa[6].h)){
        if(fase < 3){ fase++; init_fase(fase); }
        else venceu = 1;
    }

    // Barulho: correr = monstro ouve
    barulho = six.correndo ? 150 : 30;
}

void update_monstro(){
    float dist = chefe.x - six.x;
    if(dist < 0) dist = -dist;
    int veJogador = 0;

    // Visao: se jogador na frente, nao escondido, e perto
    if(!six.escondido && dist < chefe.visao && six.y > chefe.y - 50 && six.y < chefe.y + 120){
        if((chefe.direcao == 1 && six.x > chefe.x) || (chefe.direcao == -1 && six.x < chefe.x)){
            veJogador = 1;
        }
    }
    // Ouvido: barulho alto
    if(dist < barulho && six.correndo && !six.escondido) veJogador = 1;

    if(veJogador){
        chefe.estado = 1;
        chefe.timer = 180; // 3 segundos perseguindo
    } else if(chefe.timer > 0){
        chefe.timer--;
        if(chefe.timer == 0) chefe.estado = 2; // procurando
    } else if(chefe.estado == 2){
        chefe.timer = 120;
        chefe.estado = 0;
    }

    if(chefe.estado == 1){
        // Persegue rapido
        if(six.x > chefe.x){ chefe.x += 3.5f; chefe.direcao = 1; }
        else { chefe.x -= 3.5f; chefe.direcao = -1; }
        // Pegou a Six?
        if(colide(six.x,six.y,six.w,six.h,chefe.x,chefe.y,chefe.w,chefe.h)){
            six.vida--;
            six.x = 50; six.y = 500; chefe.x = 900;
            if(six.vida <= 0) gameOver = 1;
        }
    } else {
        // Patrulha devagar
        chefe.x += 1.2f * chefe.direcao;
        if(chefe.x < 100) chefe.direcao = 1;
        if(chefe.x > 1100) chefe.direcao = -1;
    }
}

int main(){
    printf("Little Nightmares: Sombra - Iniciando...\n");
    init_fase(1);

    // Loop principal 60fps
    while(!gameOver && !venceu){
        // Aqui entra o RSX pra desenhar: fundo escuro, Six amarela, monstro, chaves
        // Pad, fisica, IA - tudo acima
        tempo++;
        // update_player e update_monstro sao chamados a cada frame com o pad real
        // sysUtilCheckCallback etc.
        break; // placeholder pra compilar no Github sem RSX
    }
    return 0;
}