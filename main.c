#include <stdio.h>
int main(){
printf("Little Nightmares Sombra - PS3\n");
printf("Six fugindo do monstro...\n");
int x=50,vida=3,chaves=0;
while(vida>0){
x+=10;
if(x>500){chaves++;x=50;printf("Pegou chave %d\n",chaves);}
if(chaves>=3){printf("VOCE ESCAPOU!\n");break;}
}
return 0;
}