#include <stdio.h>
#include <stdlib.h>
#include "parshell.h"
#include "commandlinereader.h"

#define ARGNUM 7						// nome_comando + 5 argumentos(maximo) + argumento NULL
#define CMDLENGTH 20
int main(int argc, char const *argv[])
{
	char *argvector[ARGNUM];
	int i;
	int child_num = 0;

	// Inicializar o vetor argvector
	for(i=0;i<ARGNUM;i++)
		argvector[i] = NULL;

	//exit
	
	return 0;
}