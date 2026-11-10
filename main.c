#include <stdio.h>
#include <stdlib.h>
int main(){
int x=50;
int vida=3;
int chaves=0;
int monstro=0;
int escondido=0;
printf("Little Nightmares Sombra - PS3\n");
printf("Six fugindo do monstro...\n");
while(vida>0){
x+=10;
monstro+=7;
if(monstro+20>=x){
if(!escondido){
printf("Monstro perto! Se escondeu!\n");
escondido=1;
monstro=0;
}
}else{
escondido=0;
}
if(x>500){
chaves++;
x=50;
printf("Pegou chave %d/3\n",chaves);
}
if(chaves>=3){
printf("VOCE ESCAPOU! ZEROU O JOGO!\n");
break;
}
}
return 0;
}