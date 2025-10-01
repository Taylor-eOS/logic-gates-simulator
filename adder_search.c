#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {G_NOT=0,G_AND=1,G_OR=2,G_XOR=3};
int MAX_GATES = 5;
int MAX_FOUND = 3;
int PROGRESS_INTERVAL = 10000000;

typedef struct {uint8_t type;uint8_t arity;uint8_t in0;uint8_t in1;uint8_t in2;} Gate;

void prepare_input_masks(uint16_t masks[4])
{
    uint16_t m0=0,m1=0,m2=0,m3=0;
    for(int idx=0;idx<16;++idx){
        int a = idx / 4;
        int b = idx % 4;
        int a1=(a>>1)&1;
        int a0=a&1;
        int b1=(b>>1)&1;
        int b0=b&1;
        if(a1) m0 |= (1u<<idx);
        if(a0) m1 |= (1u<<idx);
        if(b1) m2 |= (1u<<idx);
        if(b0) m3 |= (1u<<idx);
    }
    masks[0]=m0;masks[1]=m1;masks[2]=m2;masks[3]=m3;
}

uint16_t eval_gate_mask(const Gate *g,const uint16_t signals[])
{
    uint16_t MASK_16 = 0xFFFF;
    if(g->type==G_NOT){
        uint16_t v = signals[g->in0];
        return (~v) & MASK_16;
    }
    if(g->arity==2){
        uint16_t a = signals[g->in0];
        uint16_t b = signals[g->in1];
        if(g->type==G_AND) return a & b;
        if(g->type==G_OR) return a | b;
        if(g->type==G_XOR) return a ^ b;
    } else {
        uint16_t a = signals[g->in0];
        uint16_t b = signals[g->in1];
        uint16_t c = signals[g->in2];
        if(g->type==G_AND) return (a & b) & c;
        if(g->type==G_OR) return (a | b) | c;
        if(g->type==G_XOR) return (a ^ b) ^ c;
    }
    return 0;
}

void make_expected_masks(uint16_t expected[3])
{
    uint16_t map_sum1=0,map_sum0=0,map_carry=0;
    for(int idx=0;idx<16;++idx){
        int a = idx / 4;
        int b = idx % 4;
        int s = a + b;
        int sum0 = s&1;
        int sum1 = (s>>1)&1;
        int carry = (s>>2)&1;
        if(sum1) map_sum1 |= (1u<<idx);
        if(sum0) map_sum0 |= (1u<<idx);
        if(carry) map_carry |= (1u<<idx);
    }
    expected[0]=map_sum1;
    expected[1]=map_sum0;
    expected[2]=map_carry;
}

int found_count=0;
long long tested_count=0;

void print_solution(const Gate gates[],int ngates,const int outputs[3])
{
    printf("\n=== Found solution with %d gates ===\n",ngates);
    printf("Inputs: A1(0), A0(1), B1(2), B0(3)\n");
    for(int i=0;i<ngates;++i){
        int signum = 4 + i;
        if(gates[i].type==G_NOT){
            printf("  S%d = NOT(S%d)\n",signum,gates[i].in0);
        } else {
            const char *op = gates[i].type==G_AND?"AND":gates[i].type==G_OR?"OR":"XOR";
            if(gates[i].arity==2){
                printf("  S%d = %s(S%d,S%d)\n",signum,op,gates[i].in0,gates[i].in1);
            } else {
                printf("  S%d = %s(S%d,S%d,S%d)\n",signum,op,gates[i].in0,gates[i].in1,gates[i].in2);
            }
        }
    }
    printf("Outputs: Sum1=S%d, Sum0=S%d, Carry=S%d\n",outputs[0],outputs[1],outputs[2]);
}

void try_outputs_and_record(const uint16_t signals[],int total_signals,const uint16_t expected[3],const Gate current_gates[],int ngates)
{
    for(int i=4;i<total_signals;++i){
        if(found_count>=MAX_FOUND) return;
        for(int j=4;j<total_signals;++j){
            if(j==i) continue;
            if(found_count>=MAX_FOUND) return;
            for(int k=4;k<total_signals;++k){
                if(k==i || k==j) continue;
                ++tested_count;
                if((tested_count % PROGRESS_INTERVAL)==0){
                    printf("  Tested %lld configurations...\n",tested_count);
                }
                if(signals[i]==expected[0] && signals[j]==expected[1] && signals[k]==expected[2]){
                    int outs[3] = {i,j,k};
                    print_solution(current_gates,ngates,outs);
                    ++found_count;
                    if(found_count>=MAX_FOUND) return;
                }
            }
        }
    }
}

void build_circuit(Gate current_gates[],int remaining,int max_gates,uint16_t signals[],int total_signals,const uint16_t expected[3])
{
    if(found_count>=MAX_FOUND) return;
    if(remaining==0){
        try_outputs_and_record(signals,total_signals,expected,current_gates,max_gates);
        return;
    }
    int available = total_signals;
    for(int t=0;t<4;++t){
        if(found_count>=MAX_FOUND) return;
        if(t==G_NOT){
            for(int i=0;i<available;++i){
                if(found_count>=MAX_FOUND) return;
                Gate g;
                g.type=G_NOT;
                g.arity=1;
                g.in0=(uint8_t)i;
                g.in1=0;
                g.in2=0;
                uint16_t out = eval_gate_mask(&g,signals);
                int duplicate=0;
                for(int s=0;s<total_signals;++s){
                    if(signals[s]==out){
                        duplicate=1;
                        break;
                    }
                }
                if(duplicate) continue;
                signals[total_signals]=out;
                current_gates[max_gates-remaining]=g;
                build_circuit(current_gates,remaining-1,max_gates,signals,total_signals+1,expected);
            }
        } else {
            for(int i=0;i<available;++i){
                if(found_count>=MAX_FOUND) return;
                for(int j=0;j<available;++j){
                    if(i==j) continue;
                    if(found_count>=MAX_FOUND) return;
                    Gate g2;
                    g2.type=(uint8_t)t;
                    g2.arity=2;
                    g2.in0=(uint8_t)i;
                    g2.in1=(uint8_t)j;
                    g2.in2=0;
                    uint16_t out2 = eval_gate_mask(&g2,signals);
                    int dup2=0;
                    for(int s=0;s<total_signals;++s){
                        if(signals[s]==out2){
                            dup2=1;
                            break;
                        }
                    }
                    if(!dup2){
                        signals[total_signals]=out2;
                        current_gates[max_gates-remaining]=g2;
                        build_circuit(current_gates,remaining-1,max_gates,signals,total_signals+1,expected);
                    }
                    for(int k=0;k<available;++k){
                        if(k==i || k==j) continue;
                        if(found_count>=MAX_FOUND) return;
                        Gate g3;
                        g3.type=(uint8_t)t;
                        g3.arity=3;
                        g3.in0=(uint8_t)i;
                        g3.in1=(uint8_t)j;
                        g3.in2=(uint8_t)k;
                        uint16_t out3 = eval_gate_mask(&g3,signals);
                        int dup3=0;
                        for(int s=0;s<total_signals;++s){
                            if(signals[s]==out3){
                                dup3=1;
                                break;
                            }
                        }
                        if(dup3) continue;
                        signals[total_signals]=out3;
                        current_gates[max_gates-remaining]=g3;
                        build_circuit(current_gates,remaining-1,max_gates,signals,total_signals+1,expected);
                    }
                }
            }
        }
    }
}

int main()
{
    uint16_t in_masks[4];
    prepare_input_masks(in_masks);
    uint16_t expected[3];
    make_expected_masks(expected);
    printf("Verifying expected masks:\n");
    for(int idx=0;idx<16;++idx){
        int a = idx/4;
        int b = idx%4;
        int sum = a+b;
        int bit0 = (expected[1]>>idx)&1;
        int bit1 = (expected[0]>>idx)&1;
        int bit2 = (expected[2]>>idx)&1;
        printf("  %d+%d=%d, got bits %d%d%d\n",a,b,sum,bit2,bit1,bit0);
    }
    printf("Expected masks: Sum1=0x%04x Sum0=0x%04x Carry=0x%04x\n",expected[0],expected[1],expected[2]);
    printf("Input masks: A1=0x%04x A0=0x%04x B1=0x%04x B0=0x%04x\n",in_masks[0],in_masks[1],in_masks[2],in_masks[3]);
    int min_gates = 5;
    int max_gates = 10;
    if(MAX_GATES>max_gates) MAX_GATES = max_gates;
    printf("\nSearching for 2-bit adder circuits...\n");
    for(int num_gates=min_gates;num_gates<=MAX_GATES;++num_gates){
        printf("\nTesting with %d gates...\n",num_gates);
        Gate *current_gates = malloc(sizeof(Gate)*num_gates);
        uint16_t *signals = malloc(sizeof(uint16_t)*(4+num_gates));
        for(int s=0;s<4;++s) signals[s]=in_masks[s];
        build_circuit(current_gates,num_gates,num_gates,signals,4,expected);
        free(current_gates);
        free(signals);
        if(found_count>0){
            printf("\nFound %d solution(s) with %d gates\n",found_count,num_gates);
            break;
        }
    }
    printf("\nTotal output configurations tested: %lld\n",tested_count);
    if(found_count==0){
        printf("No solutions found with up to %d gates\n",MAX_GATES);
    }
    return 0;
}
