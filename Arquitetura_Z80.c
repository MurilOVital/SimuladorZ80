// Simulador do microprocessador z80
// Murilo Vital Rondina
// Rafael Ribas de Lima

#include <stdio.h>
#include <stdlib.h>

#define size 9
#define Mem_size 0xFFFF

typedef struct
{
    unsigned char A, B, C, D, E, H, L;
    unsigned int PC, SP;
    unsigned char memoria[Mem_size];
} Z80;

void Inicia(Z80 *cpu)
{
    cpu->A = cpu->B = cpu->C = cpu->D = cpu->E = cpu->H = cpu->L = cpu->PC = 0;
    cpu->SP = Mem_size;
}

unsigned char Ler_Bytes_Gambi(FILE *arq)
{
    unsigned char byte;
    char bit[size];

    fgets(bit, size, arq);
    byte = strtol(bit, NULL, 2);

    return byte;
}

unsigned char Carregar_Memoria(Z80 *cpu, FILE *arq)
{
    unsigned int i = 0;

    fseek(arq, 0, SEEK_END);
    long tam_arq = ftell(arq) - 1;
    rewind(arq);

    fseek(arq, 0, SEEK_SET);

    while (ftell(arq) < tam_arq)
    {
        cpu->memoria[i] = Ler_Bytes_Gambi(arq);
        i++;
    }
    cpu->PC = 0x0;
    return i;
}

unsigned int Verifica_Endereco(unsigned char hex)
{
    unsigned char mascara_10000000 = 0x80;

    if ((hex & mascara_10000000) == mascara_10000000)
    {
        unsigned char hex_negativo = 0x0;
        unsigned int mascara_00000001 = 0x1;
        unsigned int i = 0x0;

        for (; (hex & (mascara_00000001 << i)) != (mascara_00000001 << i); i++)
            ;

        hex_negativo = (mascara_00000001 << i);
        i++;

        for (; (mascara_00000001 << i) != 0x100; i++)
        {
            if ((hex & (mascara_00000001 << i)) != (mascara_00000001 << i))
            {
                hex_negativo |= (mascara_00000001 << i);
            }
            else
            {
                hex_negativo &= ~(mascara_00000001 << i);
            }
        }
        return -hex_negativo;
    }
    else
    {
        return hex;
    }
}

unsigned char *Verifica_Regis(Z80 *cpu, unsigned char regis)
{
    switch (regis)
    {
    case 0x7:
        printf("A ");
        return &cpu->A;
        break;

    case 0x0:
        printf("B ");
        return &cpu->B;
        break;

    case 0x1:
        printf("C ");
        return &cpu->C;
        break;

    case 0x2:
        printf("D ");
        return &cpu->D;
        break;

    case 0x3:
        printf("E ");
        return &cpu->E;
        break;

    case 0x4:
        printf("H ");
        return &cpu->H;
        break;

    case 0x5:
        printf("L ");
        return &cpu->L;
        break;
    }
    return 0;
}

void Ler_Memoria(Z80 *cpu, unsigned int indice)
{
    unsigned char mascara_11000111 = 0xC7;
    unsigned char mascara_00111000 = 0x38;
    unsigned char mascara_11000000 = 0xC0;
    unsigned char mascara_00000111 = 0x07;
    unsigned char mascara_11111000 = 0xF8;
    unsigned char *registrador_1, *registrador_2;

    while (cpu->PC < indice)
    {
        switch (cpu->memoria[cpu->PC] & mascara_11000111)
        {
        case 0x06: // ld immediate
            registrador_1 = Verifica_Regis(cpu, (cpu->memoria[cpu->PC] & mascara_00111000) >> 3);
            cpu->PC++;
            *registrador_1 = cpu->memoria[cpu->PC];
            printf("Load Immediate: %d\n", *registrador_1);
            cpu->PC++;
            break;

        default:
            switch (cpu->memoria[cpu->PC] & mascara_11000000)
            {
            case 0x40: // ld regis
                registrador_1 = Verifica_Regis(cpu, (cpu->memoria[cpu->PC] & mascara_00111000) >> 3);
                registrador_2 = Verifica_Regis(cpu, (cpu->memoria[cpu->PC] & mascara_00000111));
                *registrador_1 = *registrador_2;
                printf("Load register: %d\n", *registrador_1);
                cpu->PC++;
                break;

            default:
                switch (cpu->memoria[cpu->PC] & mascara_11111000)
                {
                case 0x80: // add
                    printf("A += ");
                    registrador_1 = Verifica_Regis(cpu, (cpu->memoria[cpu->PC] & mascara_00000111));
                    cpu->A += (*registrador_1);
                    printf(": %d\n", cpu->A);
                    cpu->PC++;
                    break;

                default:
                    switch (cpu->memoria[cpu->PC])
                    {
                    case 0x32: // LD (nn), A
                        printf("LD (nn), A: ");
                        cpu->memoria[(cpu->memoria[cpu->PC + 1] << 8) | cpu->memoria[cpu->PC + 2]] = cpu->A;
                        printf("%d\n", cpu->A);
                        for (unsigned long i = 0x0; i < 1E8; i++)
                            ;
                        cpu->PC++;
                        cpu->PC++;
                        cpu->PC++;
                        break;

                    case 0xFE: // cp
                        cpu->PC++;
                        if (cpu->A == (cpu->memoria[cpu->PC]))
                        {
                            cpu->C = 1;
                        }
                        else
                            cpu->C = 0;
                        cpu->PC++;
                        printf("Compara\n");
                        break;

                    case 0x38: // jr c
                        if (cpu->C == 1)
                        {
                            cpu->PC += Verifica_Endereco(cpu->memoria[cpu->PC + 1]);
                            printf("Jump Condicional: %d\n", cpu->PC);
                        }
                        else
                        {
                            cpu->PC++;
                            cpu->PC++;
                        }
                        break;

                    case 0x18: // jr
                        cpu->PC += Verifica_Endereco(cpu->memoria[cpu->PC + 1]);
                        printf("Jump: %d\n", cpu->PC);
                        break;

                    case 0xCD: // CALL
                        cpu->memoria[cpu->SP - 1] = cpu->PC + 3;
                        cpu->PC = (cpu->memoria[cpu->PC + 1] << 8) | cpu->memoria[cpu->PC + 2];
                        printf("Call: %d\n", cpu->PC);
                        break;

                    case 0xC9: // RET
                        cpu->PC = cpu->memoria[cpu->SP - 1];
                        printf("Ret: %d\n", cpu->PC);
                        break;

                    default:
                        printf("Opcode invalido\n");
                        return;
                        break;
                    }
                    break;
                }
                break;
            }
            break;
        }
    }
}

int main()
{
    FILE *arq = fopen("binario.bin", "rb");

    if (arq == NULL)
    {
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    Z80 cpu;

    Inicia(&cpu);

    unsigned int indice = Carregar_Memoria(&cpu, arq);

    fclose(arq);

    Ler_Memoria(&cpu, indice);

    return 0;
}
