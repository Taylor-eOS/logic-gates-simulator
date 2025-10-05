#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

enum {G_NOT=0,G_AND=1,G_OR=2,G_XOR=3};
typedef struct {uint8_t type;uint8_t arity;uint8_t in0;uint8_t in1;} Gate;

uint8_t eval_gate_mask(const Gate *g,const uint8_t signals[]){
    if(g->type==G_NOT){uint8_t v=signals[g->in0];return (~v)&0xF;}
    if(g->arity==2){uint8_t a=signals[g->in0];uint8_t b=signals[g->in1];
        if(g->type==G_AND) return a & b;
        if(g->type==G_OR) return a | b;
        if(g->type==G_XOR) return a ^ b;
    }
    return 0;
}

void print_solution(const Gate *gates,int ngates,int sum_idx,int carry_idx){
    printf("\nSolution with %d gates\n",ngates);
    printf("Inputs: S0=A, S1=B\n");
    for(int i=0;i<ngates;++i){
        int out = 2 + i;
        if(gates[i].type==G_NOT) printf("  S%d = NOT(S%d)\n",out,gates[i].in0);
        else{
            const char *op = gates[i].type==G_AND?"AND":gates[i].type==G_OR?"OR":"XOR";
            printf("  S%d = %s(S%d,S%d)\n",out,op,gates[i].in0,gates[i].in1);
        }
    }
    printf("Outputs: Sum=S%d Carry=S%d\n",sum_idx,carry_idx);
}

int found_count=0;
long long checked_pairs=0;
const long long PROGRESS_INTERVAL = 100000;

void try_check_pairs(const uint8_t signals[],int total_signals,uint8_t sum_mask,uint8_t carry_mask,const Gate *gates,int ngates){
    for(int i=0;i<total_signals;++i){
        for(int j=0;j<total_signals;++j){
            if(i==j) continue;
            ++checked_pairs;
            if((checked_pairs%PROGRESS_INTERVAL)==0) printf("Checked %lld pairs...\n",checked_pairs);
            if(signals[i]==sum_mask && signals[j]==carry_mask){
                print_solution(gates,ngates,i,j);
                ++found_count;
                return;
            }
        }
    }
}

void build_circuit(Gate gates[],int depth,int max_depth,uint8_t signals[],int total_signals,uint8_t sum_mask,uint8_t carry_mask){
    if(found_count>0) return;
    try_check_pairs(signals,total_signals,sum_mask,carry_mask,gates,max_depth-depth);
    if(found_count>0) return;
    if(depth==0) return;
    int avail = total_signals;
    for(int t=0;t<4;++t){
        if(found_count>0) return;
        if(t==G_NOT){
            for(int i=0;i<avail;++i){
                Gate g={.type=G_NOT,.arity=1,.in0=(uint8_t)i,.in1=0};
                uint8_t out = eval_gate_mask(&g,signals);
                int dup=0;
                for(int s=0;s<total_signals;++s) if(signals[s]==out){dup=1;break;}
                if(dup) continue;
                signals[total_signals]=out;
                gates[max_depth-depth]=g;
                build_circuit(gates,depth-1,max_depth,signals,total_signals+1,sum_mask,carry_mask);
                if(found_count>0) return;
            }
        } else {
            for(int i=0;i<avail;++i){
                for(int j=0;j<avail;++j){
                    if(i==j) continue;
                    Gate g={.type=(uint8_t)t,.arity=2,.in0=(uint8_t)i,.in1=(uint8_t)j};
                    uint8_t out = eval_gate_mask(&g,signals);
                    int dup=0;
                    for(int s=0;s<total_signals;++s) if(signals[s]==out){dup=1;break;}
                    if(dup) continue;
                    signals[total_signals]=out;
                    gates[max_depth-depth]=g;
                    build_circuit(gates,depth-1,max_depth,signals,total_signals+1,sum_mask,carry_mask);
                    if(found_count>0) return;
                }
            }
        }
    }
}

int main(){
    uint8_t A_mask = 0xC;
    uint8_t B_mask = 0xA;
    uint8_t sum_mask = A_mask ^ B_mask;
    uint8_t carry_mask = A_mask & B_mask;
    printf("Half adder search (Sum=A XOR B, Carry=A AND B)\n");
    printf("Masks: A=0x%02x B=0x%02x Sum=0x%02x Carry=0x%02x\n",A_mask,B_mask,sum_mask,carry_mask);
    printf("Truth table verification:\n");
    for(int idx=0;idx<4;++idx){
        int a=(idx>>1)&1;
        int b=idx&1;
        int s=((sum_mask>>idx)&1);
        int c=((carry_mask>>idx)&1);
        printf("  %d + %d -> Sum=%d Carry=%d\n",a,b,s,c);
    }
    int min_gates=1;
    int max_gates=6;
    for(int ng=min_gates;ng<=max_gates;++ng){
        printf("\nTrying up to %d gates...\n",ng);
        Gate *gates = malloc(sizeof(Gate)*ng);
        uint8_t *signals = malloc(sizeof(uint8_t)*(2+ng));
        signals[0]=A_mask;
        signals[1]=B_mask;
        build_circuit(gates,ng,ng,signals,2,sum_mask,carry_mask);
        free(gates);
        free(signals);
        if(found_count>0){
            printf("\nFound %d solution(s) using up to %d gates\n",found_count,ng);
            break;
        }
    }
    if(found_count==0) printf("\nNo solution found up to %d gates\n",max_gates);
    return 0;
}

