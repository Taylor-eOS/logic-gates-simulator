#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {G_NOT=0,G_AND=1,G_OR=2,G_XOR=3};
int MAX_GATES = 9;
int MAX_FOUND = 3;
int PROGRESS_INTERVAL = 10000000;

typedef struct {uint8_t type;uint8_t arity;uint8_t in0;uint8_t in1;uint8_t in2;} Gate;

void prepare_input_masks(uint16_t masks[4])
{
    uint16_t m0=0,m1=0,m2=0,m3=0;
    int idx=0;
    for(int a=0;a<4;++a){
        int a1=(a>>1)&1;
        int a0=a&1;
        for(int b=0;b<4;++b){
            int b1=(b>>1)&1;
            int b0=b&1;
            if(a1) m0 |= (1u<<idx);
            if(a0) m1 |= (1u<<idx);
            if(b1) m2 |= (1u<<idx);
            if(b0) m3 |= (1u<<idx);
            ++idx;
        }
    }
    masks[0]=m0;masks[1]=m1;masks[2]=m2;masks[3]=m3;
}

uint16_t eval_gate_mask(const Gate *g,const uint16_t signals[])
{
    if(g->type==G_NOT){
        uint16_t v = signals[g->in0];
        uint16_t full = 0xFFFFu;
        return (~v) & full;
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

int masks_equal(uint16_t a,uint16_t b){return a==b;}

int make_expected_masks(uint16_t expected[3],const uint16_t in_masks[4])
{
    uint16_t map_sum=0,map_sumlow=0,map_carry=0;
    int idx=0;
    for(int a=0;a<4;++a){
        int a1=(a>>1)&1;
        int a0=a&1;
        for(int b=0;b<4;++b){
            int b1=(b>>1)&1;
            int b0=b&1;
            int s = a + b;
            int sum1 = (s>>1)&1;
            int sum0 = s&1;
            int carry = (s>>2)&1;
            if(sum1) map_sum |= (1u<<idx);
            if(sum0) map_sumlow |= (1u<<idx);
            if(carry) map_carry |= (1u<<idx);
            ++idx;
        }
    }
    expected[0]=map_sum;
    expected[1]=map_sumlow;
    expected[2]=map_carry;
    return 0;
}

int found_count=0;
long long tested_count=0;

void print_solution(const Gate gates[],int ngates,const int outputs[3])
{
    printf("\nFound solution with %d gates\n",ngates);
    printf("Inputs: A1(0), A0(1), B1(2), B0(3)\n");
    for(int i=0;i<ngates;++i){
        int signum = 4 + i;
        if(gates[i].type==G_NOT){
            printf("  S%d = NOT(S%d)\n",signum,gates[i].in0);
        } else {
            if(gates[i].arity==2) printf("  S%d = %s(S%d,S%d)\n",signum, gates[i].type==G_AND?"AND":gates[i].type==G_OR?"OR":"XOR",gates[i].in0,gates[i].in1);
            else printf("  S%d = %s(S%d,S%d,S%d)\n",signum, gates[i].type==G_AND?"AND":gates[i].type==G_OR?"OR":"XOR",gates[i].in0,gates[i].in1,gates[i].in2);
        }
    }
    printf("Outputs: Sum1=S%d Sum0=S%d Carry=S%d\n",outputs[0],outputs[1],outputs[2]);
}

void try_outputs_and_record(const uint16_t signals[],int total_signals,const uint16_t expected[3],const Gate current_gates[],int ngates)
{
    int start = 4;
    int end = total_signals;
    for(int i=start;i<end;++i){
        for(int j=start;j<end;++j){
            if(j==i) continue;
            for(int k=start;k<end;++k){
                if(k==i || k==j) continue;
                ++tested_count;
                if((tested_count % PROGRESS_INTERVAL)==0) printf("  Tested %lld configurations...\n",tested_count);
                if(masks_equal(signals[i],expected[0]) && masks_equal(signals[j],expected[1]) && masks_equal(signals[k],expected[2])){
                    int outs[3] = {i,j,k};
                    print_solution(current_gates,ngates,outs);
                    ++found_count;
                    if(found_count>=MAX_FOUND) return;
                }
            }
            if(found_count>=MAX_FOUND) return;
        }
        if(found_count>=MAX_FOUND) return;
    }
}

void build_circuit(Gate current_gates[],int remaining,int max_gates,uint16_t signals[],int total_signals,const uint16_t expected[3])
{
    if(found_count>=MAX_FOUND) return;
    if(remaining==0){
        try_outputs_and_record(signals,total_signals,expected,current_gates,max_gates-remaining);
        return;
    }
    int available = total_signals;
    for(int t=0;t<4;++t){
        if(found_count>=MAX_FOUND) return;
        if(t==G_NOT){
            for(int i=0;i<available;++i){
                Gate g; g.type=G_NOT; g.arity=1; g.in0=i;
                uint16_t out = eval_gate_mask(&g,signals);
                int duplicate=0;
                for(int s=0;s<total_signals;++s) if(signals[s]==out){ duplicate=1; break; }
                if(duplicate) continue;
                signals[total_signals]=out;
                current_gates[max_gates-remaining]=g;
                build_circuit(current_gates,remaining-1,max_gates,signals,total_signals+1,expected);
                if(found_count>=MAX_FOUND) return;
            }
        } else {
            for(int i=0;i<available;++i){
                for(int j=i+1;j<available;++j){
                    Gate g2; g2.type=t; g2.arity=2; g2.in0=i; g2.in1=j;
                    uint16_t out2 = eval_gate_mask(&g2,signals);
                    int dup2=0;
                    for(int s=0;s<total_signals;++s) if(signals[s]==out2){ dup2=1; break; }
                    if(!dup2){
                        signals[total_signals]=out2;
                        current_gates[max_gates-remaining]=g2;
                        build_circuit(current_gates,remaining-1,max_gates,signals,total_signals+1,expected);
                        if(found_count>=MAX_FOUND) return;
                    }
                    for(int k=j+1;k<available;++k){
                        Gate g3; g3.type=t; g3.arity=3; g3.in0=i; g3.in1=j; g3.in2=k;
                        uint16_t out3 = eval_gate_mask(&g3,signals);
                        int dup3=0;
                        for(int s=0;s<total_signals;++s) if(signals[s]==out3){ dup3=1; break; }
                        if(dup3) continue;
                        signals[total_signals]=out3;
                        current_gates[max_gates-remaining]=g3;
                        build_circuit(current_gates,remaining-1,max_gates,signals,total_signals+1,expected);
                        if(found_count>=MAX_FOUND) return;
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
    make_expected_masks(expected,in_masks);
    int min_gates = 6;
    int max_gates = 10;
    if(MAX_GATES>max_gates) MAX_GATES = max_gates;
    printf("Optimized 2-bit adder search (mask-parallel C)\n");
    for(int num_gates=min_gates;num_gates<=MAX_GATES;++num_gates){
        printf("Testing %d gates...\n",num_gates);
        Gate *current_gates = malloc(sizeof(Gate)*num_gates);
        int base_signals = 4;
        int total_signals = base_signals;
        uint16_t *signals = malloc(sizeof(uint16_t)*(base_signals+num_gates));
        for(int s=0;s<4;++s) signals[s]=in_masks[s];
        build_circuit(current_gates,num_gates,num_gates,signals,total_signals,expected);
        free(current_gates);
        free(signals);
        if(found_count>0) break;
    }
    printf("Total configurations tested: %lld\n",tested_count);
    return 0;
}

