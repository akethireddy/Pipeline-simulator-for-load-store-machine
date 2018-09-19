#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>


struct registers
{
	int data;
	int alu_data;
	int status;//0->free in WB and 1->not free
	int src_status; //0->free in alu and 1->not free
}R[16];

int zero=0;
int alu_zero=-1;
int cycle; //cycle of the pipeline 
int memory[1000]; //data in memory each of 4B
int num_lines;
char fileLines[][50]={0};
int instr_position[5][1][2];
int instr_tracker=1;
int stalling;
int stalling_tracking;
int instr_loop=-1;
int opDependency[16];

struct Instruction_Info
{
	char opcode[10];
	int source_registers[2];
	int dest_register;
	int target_memory_address;
	int target_memory_data;
	int literal;
	int instr_n;
	int value;
	int src_val[2];
}f_ip,f_op,d_ip,d_op,mul1_ip,mul1_op,mul2_ip,mul2_op,m_ip,m_op,wb_ip,wb_op,alu_ip,alu_op,div1_ip,div1_op,div2_ip,div2_op,div3_ip,div3_op,div4_ip,div4_op;


void initialize(struct Instruction_Info *s)
{
	strcpy(s->opcode,"");
	s->source_registers[0]=0;
	s->source_registers[1]=0;
	s->dest_register=0;
	s->target_memory_address=0;
	s->target_memory_data=0;
	s->literal=0;
	s->instr_n=num_lines;
	s->value=0;
	s->src_val[0]=0;
	s->src_val[1]=0;
}
/*can be used for stage to stage transfer and i/p to o/p tansfer */
void inputTOoutput(struct Instruction_Info *ip,struct Instruction_Info *op)
{
	//printf("\n@@@@@@in INPUT TO OP\n");
	strcpy(op->opcode,ip->opcode);
	op->source_registers[0]=ip->source_registers[0];
	op->source_registers[1]=ip->source_registers[1];
	op->dest_register=ip->dest_register;
	op->target_memory_address=ip->target_memory_address;
	op->target_memory_data=ip->target_memory_data;
	op->literal=ip->literal;
	op->instr_n=ip->instr_n;
	op->value=ip->value;
	op->src_val[0]=ip->src_val[0];
	op->src_val[1]=ip->src_val[1];
	initialize(ip);
}

void printing(struct Instruction_Info *ip)
{
	if(strcmp(ip->opcode,"MOVC")==0)
	{
		printf(" (I%d) %s R%d ,#%d",ip->instr_n,ip->opcode,ip->dest_register,ip->literal);
	}
	else if(strcmp(ip->opcode,"BNZ")==0 ||strcmp(ip->opcode,"BZ")==0)
	{
		printf(" (I%d) %s #%d",ip->instr_n,ip->opcode,ip->literal);
	}
	else if(strcmp(ip->opcode,"JUMP")==0)
	{
		printf(" (I%d) %s R%d ,#%d",ip->instr_n,ip->opcode,ip->dest_register,ip->literal);
	}
	else if(strcmp(ip->opcode,"HALT")==0)
	{
		printf(" (I%d) %s",ip->instr_n,ip->opcode);
	}

	else if(strcmp(ip->opcode,"JAL")==0)
	{
		printf(" (I%d) %s R%d,R%d,#%d",ip->instr_n,ip->opcode,ip->dest_register,ip->source_registers[0],ip->literal);
	}
	else
	{
		printf(" (I%d) %s R%d, R%d, R%d",ip->instr_n,ip->opcode,ip->dest_register,ip->source_registers[0],ip->source_registers[1]);
	}

}

/*can be used for stage to stage transfer and i/p to o/p tansfer */
void fetchinputTOoutput(struct Instruction_Info *ip,struct Instruction_Info *op)
{
	strcpy(op->opcode,ip->opcode);
	op->source_registers[0]=ip->source_registers[0];
	op->source_registers[1]=ip->source_registers[1];
	op->dest_register=ip->dest_register;
	op->target_memory_address=ip->target_memory_address;
	op->target_memory_data=ip->target_memory_data;
	op->literal=ip->literal;
	op->instr_n=ip->instr_n;
	op->value=ip->value;
	op->src_val[0]=ip->src_val[0];
	op->src_val[1]=ip->src_val[1];

}
void tokens(char *opcode,int *source_registers,int* dest_register,int* target_memory_address,int* target_memory_data,int *literal,int line)
{

char *token;
char t[25];
token=strtok(fileLines[line],"#, ");
strcpy(opcode,token);
//printf("\nopcode=%s",opcode);
if(strcmp(opcode,"HALT")==0)
{
	//printf("\ninside else if\n");
	*literal=0;
	*dest_register=50;
	source_registers[0]=50;
	source_registers[1]=50;

}

else if(strcmp(opcode,"BNZ")!=0 || strcmp(opcode,"BZ")!=0)
{
	//printf("\nINSIDE IF\n");
	token=strtok(NULL,"");//this token value will have remaining instruction except the opcode


	strcpy(t,token);//the remaininig instruction except the opcode is being copied into t char pointer

	int flag=0;
	token=strtok(t,"#");
	//printf("\ntoken next val:%s\n",token);
	//char regs[20];
	//strcpy(regs,token);//this holds the register strings

	char *regs;
	regs=token;//this holds the string of registers in an insruction


	if(token=strtok(NULL,""))//this holds the literal value which should be converted using atoi
	{
		*literal=atoi(token);//this has the literal value if any 
	}
	else
	{
		*literal=0;
	}
 
	//printf("\nliteral=%d",*literal); 
	char *p;
	p=regs; //p has the string of source and destination regsiters 
//	printf("\nreg str last :%s",p);
	int check=0;
	int index=0;
	while(*p)
	{
		//if(isdigit(*p))//even for STORE this stores the source as destination, but while actually executing instruction consider source as destination and destination as source
		//{
		if((p[0] == '-' && isdigit(p[1]))|| isdigit(p[0]))
		{
		long value=strtol(p,&p,10);
		if(check==0) //this should differ for store as the first register value is store in the calculated address. the checking of type of opcode should be done here
		{
			*dest_register=value;
			if(strcmp(opcode,"BNZ")==0)
			{
				*literal=*dest_register;
				*dest_register=50;

			}
			if(strcmp(opcode,"BZ")==0)
			{
				*literal=*dest_register;
				*dest_register=50;

			}

			check++;
		}
		else
		{
			source_registers[index]=value;
			index++;
		}
	}
	else
		p++;
	}//end of while

	//printf("\nsource registers =%d ,%d \n",source_registers[0],source_registers[1]);
	//printf("\ndest register= %d\n",*dest_register);

}

else
{
	token=strtok(NULL,"");//this token value will have remaining instruction except the opcode

	if(token=strtok(token,"#"))//this holds the literal value which should be converted using atoi
	{
		*literal=atoi(token);//this has the literal value if any 
		*dest_register=-1;
	}
	else
	{
		*literal=0;
	}

}

}



void fetch()
{
	printf("\nFETCH :");
	//printing(&f_ip);
	if(strlen(f_op.opcode)==0)
	{
		printing(&f_ip);
		inputTOoutput(&f_ip,&f_op);
		instr_tracker++;
	}
	else
		printing(&f_op);
	//printing(&f_op);
}


void decode()
{
	printf("\nDECODE :");
	if(strlen(d_ip.opcode)==0)
	{
		//printf("\nIN DECODE-IF\n");
		if(strlen(d_op.opcode)!=0)
		printing(&d_op); 
		return;
	}
	else
	{
		//printf("\nIN DECODE-ELSE\n");
		printing(&d_ip);
		if(strcmp(d_ip.opcode,"ADD")==0 || strcmp(d_ip.opcode,"MUL")==0 || strcmp(d_ip.opcode,"SUB")==0 || strcmp(d_ip.opcode,"AND")==0 || strcmp(d_ip.opcode,"EXOR")==0 || strcmp(d_ip.opcode,"OR")==0 || strcmp(d_ip.opcode,"LOAD")==0 || strcmp(d_ip.opcode,"STORE")==0 ||strcmp(d_ip.opcode,"DIV")==0)
		{
			//printf("\nIN DECODE-ELSE-IF\n");
			//printf("\nR[%d].src_status=%d\nR[%d].src_status=%d\nR[%d].src_status=%d\n",d_ip.source_registers[0],R[d_ip.source_registers[0]].src_status,d_ip.source_registers[1],R[d_ip.source_registers[1]].src_status,d_ip.dest_register,R[d_ip.dest_register].src_status);
			if(R[d_ip.source_registers[0]].src_status==0 && R[d_ip.source_registers[1]].src_status==0 && R[d_ip.dest_register].src_status==0 && strlen(d_op.opcode)==0)
			{
				if(opDependency[d_ip.dest_register]==0)
				{
					d_ip.src_val[0]=R[d_ip.source_registers[0]].alu_data;
					d_ip.src_val[1]=R[d_ip.source_registers[1]].alu_data;
					inputTOoutput(&d_ip,&d_op);
				}
			}
			else
			{
				//printf("\nIN DECODE-ELSE-ELSE\n");
				//printing(&d_op);
				return;
			}
		}
		else if(strcmp(d_ip.opcode,"JAL")==0)
		{
			if(R[d_ip.source_registers[0]].src_status==0 && R[d_ip.dest_register].src_status==0 && strlen(d_op.opcode)==0)
			{
				if(opDependency[d_ip.dest_register]==0)
				{
					d_ip.src_val[0]=R[d_ip.source_registers[0]].alu_data;
					inputTOoutput(&d_ip,&d_op);
				}
			}
		}
		else if(strcmp(d_ip.opcode,"MOVC")==0 || strcmp(d_ip.opcode,"JUMP")==0)
		{
			if(R[d_ip.dest_register].src_status==0 && strlen(d_op.opcode)==0)
			{
				inputTOoutput(&d_ip,&d_op);		
			}
			else
				return;
		}
		else if(strcmp(d_ip.opcode,"HALT")==0)
		{
			if(strlen(d_op.opcode)==0)
			{
					inputTOoutput(&d_ip,&d_op);
			}
			else
				return;
		}
		else if(strcmp(d_ip.opcode,"BNZ")==0 || strcmp(d_ip.opcode,"BZ")==0)
		{
			if(strlen(d_op.opcode)==0)
			{
				inputTOoutput(&d_ip,&d_op);		
			}
			else
				return;
		}

	}

	//printing(&d_op);

}

void alu()
{
	printf("\nALU :");
	if(strlen(alu_ip.opcode)==0)
	{
		if(strlen(alu_op.opcode)!=0)
		printing(&alu_op);	
		return;
	}
	else
	{
		if(strcmp(alu_ip.opcode,"ADD")==0)
		{
			//alu_ip.value=alu_ip.src_val[0]+alu_ip.src_val[1];

			alu_ip.value=R[alu_ip.source_registers[0]].alu_data+R[alu_ip.source_registers[1]].alu_data;

			if(alu_ip.value==0)
				alu_zero=1;
			else
				alu_zero=0;

			R[alu_ip.source_registers[0]].src_status=0;
			R[alu_ip.source_registers[1]].src_status=0;
			if(d_ip.source_registers[0]==alu_ip.dest_register || d_ip.source_registers[1]==alu_ip.dest_register)
		    {
				R[alu_ip.dest_register].src_status=0;
			}
			opDependency[alu_ip.dest_register]=1;
			R[alu_ip.dest_register].alu_data=alu_ip.value;
			inputTOoutput(&alu_ip,&alu_op);
		}

		if(strcmp(alu_ip.opcode,"MOVC")==0)
		{
			//printf("\nin movc\n");
			alu_ip.value=alu_ip.literal+0;
			//R[alu_ip.source_registers[0]].src_status=0;
			//R[alu_ip.source_registers[1]].src_status=0;
			if(d_ip.source_registers[0]==alu_ip.dest_register || d_ip.source_registers[1]==alu_ip.dest_register)
			{
				R[alu_ip.dest_register].src_status=0;
			}
			opDependency[alu_ip.dest_register]=1;
			R[alu_ip.dest_register].alu_data=alu_ip.value;
			inputTOoutput(&alu_ip,&alu_op);
		}
		if(strcmp(alu_ip.opcode,"SUB")==0)
		{
			alu_ip.value=alu_ip.src_val[0]-alu_ip.src_val[1];

			if(alu_ip.value==0)
			{
				alu_zero=1;
			}
			else
				alu_zero=0;

			R[alu_ip.source_registers[0]].src_status=0;
			R[alu_ip.source_registers[1]].src_status=0;
			if(d_ip.source_registers[0]==alu_ip.dest_register || d_ip.source_registers[1]==alu_ip.dest_register)
			{
				R[alu_ip.dest_register].src_status=0;
			}
			opDependency[alu_ip.dest_register]=1;
			R[alu_ip.dest_register].alu_data=alu_ip.value;
			inputTOoutput(&alu_ip,&alu_op);
		}
		if(strcmp(alu_ip.opcode,"AND")==0)
		{
			alu_ip.value=alu_ip.src_val[0] & alu_ip.src_val[1];
			R[alu_ip.source_registers[0]].src_status=0;
			R[alu_ip.source_registers[1]].src_status=0;
			if(d_ip.source_registers[0]==alu_ip.dest_register || d_ip.source_registers[1]==alu_ip.dest_register)
			{
				R[alu_ip.dest_register].src_status=0;
			}
			opDependency[alu_ip.dest_register]=1;
			R[alu_ip.dest_register].alu_data=alu_ip.value;
			inputTOoutput(&alu_ip,&alu_op);
		}
		if(strcmp(alu_ip.opcode,"EXOR")==0)
		{
			alu_ip.value=alu_ip.src_val[0] ^ alu_ip.src_val[1];
			R[alu_ip.source_registers[0]].src_status=0;
			R[alu_ip.source_registers[1]].src_status=0;
			if(d_ip.source_registers[0]==alu_ip.dest_register || d_ip.source_registers[1]==alu_ip.dest_register)
			{
				R[alu_ip.dest_register].src_status=0;
			}
			opDependency[alu_ip.dest_register]=1;
			R[alu_ip.dest_register].alu_data=alu_ip.value;
			inputTOoutput(&alu_ip,&alu_op);
		}
		if(strcmp(alu_ip.opcode,"OR")==0)
		{
			alu_ip.value=alu_ip.src_val[0] | alu_ip.src_val[1];
			R[alu_ip.source_registers[0]].src_status=0;
			R[alu_ip.source_registers[1]].src_status=0;
			if(d_ip.source_registers[0]==alu_ip.dest_register || d_ip.source_registers[1]==alu_ip.dest_register)
			{
				R[alu_ip.dest_register].src_status=0;
			}
			opDependency[alu_ip.dest_register]=1;
			R[alu_ip.dest_register].alu_data=alu_ip.value;
			inputTOoutput(&alu_ip,&alu_op);
		}
		if(strcmp(alu_ip.opcode,"LOAD")==0)
		{
			alu_ip.value=alu_ip.src_val[0]+alu_ip.literal;
			alu_ip.value=alu_ip.value/4;
			R[alu_ip.source_registers[0]].src_status=0;
			R[alu_ip.source_registers[1]].src_status=0;
			if(d_ip.source_registers[0]==alu_ip.dest_register || d_ip.source_registers[1]==alu_ip.dest_register)
			{
				R[alu_ip.dest_register].src_status=0;
			}
			opDependency[alu_ip.dest_register]=1;
			R[alu_ip.dest_register].alu_data=alu_ip.value;
			inputTOoutput(&alu_ip,&alu_op);
		}
		if(strcmp(alu_ip.opcode,"STORE")==0)
		{
			alu_ip.value=alu_ip.src_val[0]+alu_ip.literal;
			alu_ip.value=alu_ip.value/4;
			R[alu_ip.source_registers[0]].src_status=0;
			R[alu_ip.source_registers[1]].src_status=0;
			R[alu_ip.dest_register].src_status=0;
			opDependency[alu_ip.dest_register]=1;
			R[alu_ip.dest_register].alu_data=alu_ip.value;
			inputTOoutput(&alu_ip,&alu_op);
		}
		if(strcmp(alu_ip.opcode,"BNZ")==0)
		{
			if(alu_zero==0)
			{
				if(alu_ip.literal==0)
				{
					instr_loop=alu_ip.instr_n+1+(alu_ip.dest_register)/4;
				}
				else
				{
					instr_loop=alu_ip.instr_n+1+(alu_ip.literal)/4;
				}
				
				initialize(&d_op);
				initialize(&d_ip);
				initialize(&f_ip);
				initialize(&f_op);
				//initialize(&mul2_op);
				//initialize(&mul2_ip);
				//initialize(&mul1_op);
				//initialize(&mul1_ip);
				inputTOoutput(&alu_ip,&alu_op);
			}
			else
			{
				inputTOoutput(&alu_ip,&alu_op);
			}

		}

		if(strcmp(alu_ip.opcode,"JAL")==0)
		{
			//printf("\nIN JAL ALU\n");
			alu_ip.value=alu_ip.instr_n+1;
			instr_loop=(((alu_ip.src_val[0]+alu_ip.literal)-4000)/4)+1;
			initialize(&d_op);
			initialize(&d_ip);
			initialize(&f_ip);
			initialize(&f_op);
			inputTOoutput(&alu_ip,&alu_op);
		}
		if(strcmp(alu_ip.opcode,"BZ")==0)
		{
			if(alu_zero==1)
			{
				if(alu_ip.literal==0)
				{
					instr_loop=alu_ip.instr_n+1+(alu_ip.dest_register)/4;
				}
				else
				{
					instr_loop=alu_ip.instr_n+1+(alu_ip.literal)/4;
				}
				
				initialize(&d_op);
				initialize(&d_ip);
				initialize(&f_ip);
				initialize(&f_op);
				//initialize(&mul2_op);
				//initialize(&mul2_ip);
				//initialize(&mul1_op);
				//initialize(&mul1_ip);
				inputTOoutput(&alu_ip,&alu_op);
			}
			else
			{
				inputTOoutput(&alu_ip,&alu_op);
			}

		}
		if(strcmp(alu_ip.opcode,"JUMP")==0)
		{
				//instr_loop=alu_ip.literal % 4000;
				//instr_loop= instr_loop/ 4;
				//instr_loop=(instr_loop+R[alu_ip.dest_register].data)/4+1;

				//printf("\ndest value=%d\nliteral val= %d\n",R[alu_ip.dest_register].alu_data,alu_ip.literal);
				instr_loop=(((R[alu_ip.dest_register].alu_data+alu_ip.literal)-4000)/4)+1;
				//printf("\nJUMP INST LOOP= %d\n",instr_loop);
				initialize(&d_op);
				initialize(&d_ip);
				initialize(&f_ip);
				initialize(&f_op);
				//initialize(&mul2_op);
				//initialize(&mul2_ip);
				//initialize(&mul1_op);
				//initialize(&mul1_ip);
				inputTOoutput(&alu_ip,&alu_op);

		}
	}
	printing(&alu_op);
}

void mul1()
{
	printf("\nMUL1 :");
	if(strlen(mul1_ip.opcode)==0)
	{
		if(strlen(mul1_op.opcode)!=0)
		printing(&mul1_op);
		return;
	}
	else
	{
		inputTOoutput(&mul1_ip,&mul1_op);
	}
	printing(&mul1_op);
}

void mul2()
{
	printf("\nMUL2 :");
	if(strlen(mul2_ip.opcode)==0)
	{
		if(strlen(mul2_op.opcode)!=0)
		printing(&mul2_op);
		return;
	}
	else
	{
		//mul2_ip.value=mul2_ip.src_val[0]*mul2_ip.src_val[1];

		mul2_ip.value=R[mul2_ip.source_registers[0]].alu_data*R[mul2_ip.source_registers[1]].alu_data;
		if(mul2_ip.value==0)
		{
			alu_zero=1;
		}
		else
			alu_zero=0;

		R[mul2_ip.source_registers[0]].src_status=0;
		R[mul2_ip.source_registers[1]].src_status=0;
		if(d_ip.source_registers[0]==mul2_ip.dest_register || d_ip.source_registers[1]==mul2_ip.dest_register)
		{
			R[mul2_ip.dest_register].src_status=0;
		}
		opDependency[mul2_ip.dest_register]=1;
		R[mul2_ip.dest_register].alu_data=mul2_ip.value;
		inputTOoutput(&mul2_ip,&mul2_op);
	}
	printing(&mul2_op);
}

void div1()
{
	printf("\nDIV1:");
	if(strlen(div1_ip.opcode)==0)
	{
		if(strlen(div1_op.opcode)!=0)
		printing(&div1_op);
		return;
	}
	else
	{
		if(strcmp(div1_ip.opcode,"HALT")==0)
		{
			initialize(&d_op);
			initialize(&d_ip);
			initialize(&f_ip);
			initialize(&f_op);
		}
		if(d_ip.source_registers[0]==div1_ip.dest_register || d_ip.source_registers[1]==div1_ip.dest_register)
			{
				R[div1_ip.dest_register].src_status=1;
			}
		inputTOoutput(&div1_ip,&div1_op);
	}
	printing(&div1_op);
}

void div2()
{
	printf("\nDIV2:");
	if(strlen(div2_ip.opcode)==0)
	{
		if(strlen(div2_op.opcode)!=0)
		printing(&div2_op);
		return;
	}
	else
	{
		if(strcmp(div2_ip.opcode,"HALT")==0)
		{
			initialize(&d_op);
			initialize(&d_ip);
			initialize(&f_ip);
			initialize(&f_op);
		}
		if(d_ip.source_registers[0]==div2_ip.dest_register || d_ip.source_registers[1]==div2_ip.dest_register)
			{
				R[div2_ip.dest_register].src_status=1;
			}
		inputTOoutput(&div2_ip,&div2_op);
	}
	printing(&div2_op);
}

void div3()
{
	printf("\nDIV3:");
	if(strlen(div3_ip.opcode)==0 || strcmp(div3_ip.opcode,"\n")==0)
	{
		//printf("\nin div3 if\n");
		if(strlen(div3_op.opcode)!=0)
		printing(&div3_op);
		return;
	}
	else
	{
		if(strcmp(div3_ip.opcode,"\n")!=0)
		{
			if(strcmp(div3_ip.opcode,"HALT")==0)
			{
				initialize(&d_op);
				initialize(&d_ip);
				initialize(&f_ip);
				initialize(&f_op);
			}
			//printf("\ninseide div3 else if\n");
			if(d_ip.source_registers[0]==div3_ip.dest_register || d_ip.source_registers[1]==div3_ip.dest_register)
			{
				R[div3_ip.dest_register].src_status=1;
			}
			inputTOoutput(&div3_ip,&div3_op);
		}
	}
	//printf("\nin div3 outside\n");
	//printf("opcode = %s",div3_op.opcode);
	printing(&div3_op);
}

void div4()
{
	printf("\nDIV4:");
	if(strlen(div4_ip.opcode)==0)
	{
		//printf("\nin div4 if\n");
		//printf("opcode = %s",div4_op.opcode);
		if(strlen(div4_op.opcode)!=0)
		printing(&div4_op);
		return;
	}
	else
	{
		if(strcmp(div4_ip.opcode,"HALT")==0)
		{
			initialize(&d_op);
			initialize(&d_ip);
			initialize(&f_ip);
			initialize(&f_op);
			inputTOoutput(&div4_ip,&div4_op);
		}
		else
		{

			//div4_ip.value=div4_ip.src_val[0]/div4_ip.src_val[1];
			div4_ip.value==R[div4_ip.source_registers[0]].alu_data/R[div4_ip.source_registers[1]].alu_data;
			if(div4_ip.value==0)
			{
				alu_zero=1;
			}
			else
				alu_zero=0;

			R[div4_ip.source_registers[0]].src_status=0;
			R[div4_ip.source_registers[1]].src_status=0;
			if(d_ip.source_registers[0]==div4_ip.dest_register || d_ip.source_registers[1]==div4_ip.dest_register)
			{
				R[div4_ip.dest_register].src_status=0;
			}
			opDependency[div4_ip.dest_register]=1;
			R[div4_ip.dest_register].alu_data=div4_ip.value;
			inputTOoutput(&div4_ip,&div4_op);
		}
	}
	//printf("\nin div4 outside\n");
	printing(&div4_op);
}

void mem()
{
	printf("\nMEM :");
	int ch;
	if(strlen(m_ip.opcode)==0)
	{
		return;
	}
	else
	{
		if(strcmp(m_ip.opcode,"ADD")==0)
			ch=0;
		else if(strcmp(m_ip.opcode,"MUL")==0)
			ch=1;
		else if(strcmp(m_ip.opcode,"MOVC")==0)
		{
			ch=2;
		}
		else if(strcmp(m_ip.opcode,"MUL")==0)
		{
			ch=3;
		}
		else if(strcmp(m_ip.opcode,"SUB")==0)
		{
			ch=4;
		}
		else if(strcmp(m_ip.opcode,"AND")==0)
		{
			ch=5;
		}
		else if(strcmp(m_ip.opcode,"EXOR")==0)
		{
			ch=6;
		}
		else if(strcmp(m_ip.opcode,"OR")==0)
		{
			ch=7;
		}
		else if(strcmp(m_ip.opcode,"LOAD")==0)
		{
			ch=8;
		}
		
		else if(strcmp(m_ip.opcode,"STORE")==0)
		{
			ch=9;
		}
		else if(strcmp(m_ip.opcode,"BNZ")==0)
		{
			ch=10;
		}
		else if(strcmp(m_ip.opcode,"BZ")==0)
		{
			ch=11;
		}
		else if(strcmp(m_ip.opcode,"JUMP")==0)
		{
			ch=12;
		}
		else if(strcmp(m_ip.opcode,"DIV")==0)
		{
			ch=13;
		}
		else if(strcmp(m_ip.opcode,"JAL")==0)
		{
			ch=14;
		}
		else if(strcmp(m_ip.opcode,"HALT")==0)
		{
			ch=15;
		}
		switch(ch)
		{	
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 13:
			case 14:
					R[m_ip.dest_register].status=1;
					R[m_ip.source_registers[0]].status=0;
					R[m_ip.source_registers[1]].status=0;
					inputTOoutput(&m_ip,&m_op);
					break;
			case 8:
					R[m_ip.dest_register].status=1;
					R[m_ip.source_registers[0]].status=0;
					R[m_ip.source_registers[1]].status=0;
					m_ip.value=memory[m_ip.value];
					inputTOoutput(&m_ip,&m_op);
					break;
			case 9:
					R[m_ip.dest_register].status=1;
					R[m_ip.source_registers[0]].status=0;
					R[m_ip.source_registers[1]].status=0;
					memory[m_ip.value]=R[m_ip.dest_register].data;
					inputTOoutput(&m_ip,&m_op);
					break;
			case 10:
			case 11:
			case 12:
			case 15:		
					inputTOoutput(&m_ip,&m_op);
					break;
			
		}
		
	}

	printing(&m_op);
}

void wb()
{
	printf("\nWB :");
	int ch;
	if(strlen(wb_ip.opcode)==0)//if there is no instruction in that stage, then return
	{
		return;
	}
	else
	{
		int p=strcmp(wb_ip.opcode,"MOVC");

		if(strcmp(wb_ip.opcode,"ADD")==0)
			ch=0;
		else if(strcmp(wb_ip.opcode,"MUL")==0)
			ch=1;
		else if(strcmp(wb_ip.opcode,"MOVC")==0)
		{
			ch=2;
		}
		else if(strcmp(wb_ip.opcode,"MUL")==0)
		{
			ch=3;
		}
		else if(strcmp(wb_ip.opcode,"SUB")==0)
		{
			ch=4;
		}
		else if(strcmp(wb_ip.opcode,"AND")==0)
		{
			ch=5;
		}
		else if(strcmp(wb_ip.opcode,"EXOR")==0)
		{
			ch=6;
		}
		else if(strcmp(wb_ip.opcode,"OR")==0)
		{
			ch=7;
		}
		else if(strcmp(wb_ip.opcode,"LOAD")==0)
		{
			ch=8;
		}
		else if(strcmp(wb_ip.opcode,"STORE")==0)
		{
			ch=9;
		}
		else if(strcmp(wb_ip.opcode,"BNZ")==0)
		{
			ch=10;
		}
		else if(strcmp(wb_ip.opcode,"BZ")==0)
		{
			ch=11;
		}
		else if(strcmp(wb_ip.opcode,"JUMP")==0)
		{
			ch=12;
		}
		else if(strcmp(wb_ip.opcode,"DIV")==0)
		{
			ch=13;
		}
		else if(strcmp(wb_ip.opcode,"JAL")==0)
		{
			ch=14;
		}
		else if(strcmp(wb_ip.opcode,"HALT")==0)
		{
			ch=15;
		}
		
		switch(ch)
		{
			case 0:
			case 1:
			case 3:
			case 4:
			case 13:
					R[wb_ip.dest_register].data=wb_ip.value;
					if(wb_ip.value==0)
						zero=1;
					
					R[wb_ip.dest_register].status=0;
					R[wb_ip.dest_register].src_status=0;
					R[wb_ip.source_registers[0]].status=0;
					R[wb_ip.source_registers[1]].status=0;
					opDependency[wb_ip.dest_register]=0;
					//printf("\nwb() stage info: R[%d].status=%d\nR[%d].src_status=%d\nR[%d].status=%d\nopDependency[%d]=%d",wb_ip.dest_register,R[wb_ip.dest_register].status,wb_ip.dest_register,R[wb_ip.dest_register].src_status,wb_ip.source_registers[0],R[wb_ip.source_registers[0]].status,wb_ip.dest_register,opDependency[wb_ip.dest_register]);
					inputTOoutput(&wb_ip,&wb_op);
					printing(&wb_op);

					initialize(&wb_op);
					break;
			case 5:
			case 6:
			case 7:
					R[wb_ip.dest_register].data=wb_ip.value;
					R[wb_ip.dest_register].alu_data=wb_ip.value;
					//printf("\n$$$$$$$ R[%d]= %d\n",wb_ip.dest_register,R[wb_ip.dest_register].alu_data);
					R[wb_ip.dest_register].status=0;
					R[wb_ip.dest_register].src_status=0;
					R[wb_ip.source_registers[0]].status=0;
					R[wb_ip.source_registers[1]].status=0;
					opDependency[wb_ip.dest_register]=0;
					inputTOoutput(&wb_ip,&wb_op);
					printing(&wb_op);
					initialize(&wb_op);
					break;
			case 14:
					R[wb_ip.dest_register].data=(wb_ip.value*4)+4000;
					R[wb_ip.dest_register].alu_data=(wb_ip.value*4)+4000;
					//printf("\n$$$$$$$ R[%d]= %d\n",wb_ip.dest_register,R[wb_ip.dest_register].alu_data);
					R[wb_ip.dest_register].status=0;
					R[wb_ip.dest_register].src_status=0;
					R[wb_ip.source_registers[0]].status=0;
					R[wb_ip.source_registers[1]].status=0;
					opDependency[wb_ip.dest_register]=0;
					inputTOoutput(&wb_ip,&wb_op);
					printing(&wb_op);
					initialize(&wb_op);
					break;
			case 2:
			case 8:
					R[wb_ip.dest_register].data=wb_ip.value;
					R[wb_ip.dest_register].status=0;
					R[wb_ip.dest_register].src_status=0;
					R[wb_ip.source_registers[0]].status=0;
					R[wb_ip.source_registers[1]].status=0;
					opDependency[wb_ip.dest_register]=0;
					inputTOoutput(&wb_ip,&wb_op);
					printing(&wb_op);
					initialize(&wb_op);
					break;

			case 9:
					R[wb_ip.dest_register].status=0;
					R[wb_ip.dest_register].src_status=0;
					R[wb_ip.source_registers[0]].status=0;
					R[wb_ip.source_registers[1]].status=0;
					opDependency[wb_ip.dest_register]=0;
					inputTOoutput(&wb_ip,&wb_op);
					printing(&wb_op);
					initialize(&wb_op);
					break;
			case 10:
			case 11:		
					inputTOoutput(&wb_ip,&wb_op);
					printing(&wb_op);
					initialize(&wb_op);
					break;
			case 12:
					R[wb_ip.dest_register].status=0;
					R[wb_ip.dest_register].src_status=0;
					opDependency[wb_ip.dest_register]=0;
					inputTOoutput(&wb_ip,&wb_op);
					printing(&wb_op);
					initialize(&wb_op);
					break;
			case 15: 
					inputTOoutput(&wb_ip,&wb_op);
					printing(&wb_op);
					initialize(&wb_op);
					//exit(0);

		}

	}
	
	for(int q=0;q<16;q++)
	{
		R[q].alu_data=R[q].data;
	}				
}

int main(int argc,char* argv[])
{

	/*-------------------------------------------------------------------------------------------------------------------------*/

	if(argc>2)
	{
		printf("enter only the simulator name and input file name with extension");
		exit(0);
	}

	//printf("\n%s",argv[1]); //argc[1] has the input file name

	FILE *fp=fopen(argv[1],"r"); //file in read mode
	int lines=0; //number of lines in the input file
	int ch;
	int size;
	if(NULL!=fp)
	{
	fseek(fp,0,SEEK_END);
	size=ftell(fp);
	if(0==size)//checks if file is empty
	{
		printf("\nfile empty\n");//if empty, then exit
		exit(0);
	}
	

	}
	fseek(fp,0,SEEK_SET);//again sets the file pointer to the start of the file
	
	
	int charsonline=0;
	while((ch=fgetc(fp))!=EOF)
	{
		if(ch=='\n')
		{
			lines++;
			charsonline=0;
		}
		else
		{
			charsonline++;
		}
	}
	if(charsonline>0)
		lines++;

	num_lines=lines;	
	fseek(fp,0,SEEK_SET);//sets the file pointer to the start of the file


	char *line=NULL;
	size_t len=0;
	ssize_t read;
	int i=0;
	while((read=getline(&line,&len,fp))!=-1)//reads each line of the input file and stores it in fileLines char array 
	{
		strcpy(fileLines[i],line);
		i++;
	}


	/*-----------------------------------------------------------------------------------------------------------------------------*/
	

	int init;
	printf("if initialize enter 1\n");
	scanf("%d",&init);



	/*--------------------UPDATING INSTUCTIONS IN STRUCTURES----------------------------------*/
	int instr_num;
	struct Instruction_Info ii[lines];
	int line_num;
	for(instr_num=1;instr_num<=lines;instr_num++)
	{
		line_num=instr_num-1;
		ii[instr_num-1].source_registers[0]=50;
		ii[instr_num-1].source_registers[1]=50;

		if(strcmp(fileLines[instr_num-1],"HALT\n")==0)
		{
			strcpy(ii[instr_num-1].opcode,"HALT");
			ii[instr_num-1].source_registers[0]=0;
			ii[instr_num-1].source_registers[1]=0;
			ii[instr_num-1].dest_register=0;
			ii[instr_num-1].target_memory_address=0;
			ii[instr_num-1].target_memory_data=0;
			ii[instr_num-1].literal=0;
		}
		else
		{
			tokens(ii[instr_num-1].opcode,ii[instr_num-1].source_registers,&ii[instr_num-1].dest_register,&ii[instr_num-1].target_memory_address,&ii[instr_num-1].target_memory_data,&ii[instr_num-1].literal,line_num);
		}
		ii[instr_num-1].instr_n=line_num;
		ii[instr_num-1].value=0;
		ii[instr_num-1].src_val[0]=0;
		ii[instr_num-1].src_val[1]=0;

	}
	


	/*------------------------------INITIALIZING THE FETCH STAGE-----------------------------*/
	
	
	if(init==1)
	{
		strcpy(f_ip.opcode,ii[0].opcode);
		f_ip.source_registers[0]=ii[0].source_registers[0];
		f_ip.source_registers[1]=ii[0].source_registers[1];
		f_ip.dest_register=ii[0].dest_register;
		f_ip.target_memory_address=ii[0].target_memory_address;
		f_ip.target_memory_data=ii[0].target_memory_data;
		f_ip.literal=ii[0].literal;
		f_ip.instr_n=ii[0].instr_n;
		f_ip.value=0;
		f_ip.src_val[0]=0;
		f_ip.src_val[1]=0;
	}


	
	/*----------------------------INITIALIZING ALL STRUCTURE INSTANCES---------------------------------------*/


	initialize(&f_op);
	initialize(&alu_ip);
	initialize(&alu_op);
	initialize(&m_ip);
	initialize(&m_op);
	initialize(&mul1_ip);
	initialize(&mul1_op);
	initialize(&mul2_ip);
	initialize(&mul2_op);
	initialize(&d_ip);
	initialize(&d_op);
	initialize(&wb_ip);
	initialize(&wb_op);
	
	
	int k=0;
	for(k=0;k<16;k++)
	{
		R[k].data=0;
		R[k].alu_data=0;
		R[k].status=0;
		R[k].src_status=0;
	}

	

	/*---------------------STARTING THE INSTCUTION EXECUTION FOR GIVEN NUMBER OF CYCLES---------------------------------------------*/

	int cycle_num;
	printf("\nEnter the number of cycles\n");
	scanf("%d",&cycle_num);
	int j;
	//f_ip.instr_n=lines;//this helps while checking which functional unit should be passed to memory stage
	d_ip.instr_n=lines;
	alu_ip.instr_n=lines;
	mul1_ip.instr_n=lines;
	mul2_ip.instr_n=lines;
	m_ip.instr_n=lines;
	wb_ip.instr_n=lines;
	d_op.instr_n=lines;
	alu_op.instr_n=lines;
	mul1_op.instr_n=lines;
	mul2_op.instr_n=lines;
	m_op.instr_n=lines;
	wb_op.instr_n=lines;
	f_op.instr_n=lines;

	div1_ip.instr_n=lines;
	div1_op.instr_n=lines;
	div2_ip.instr_n=lines;
	div2_op.instr_n=lines;
	div3_ip.instr_n=lines;
	div3_op.instr_n=lines;
	div4_ip.instr_n=lines;
	div4_op.instr_n=lines;

int t=0;
	for(j=0;j<cycle_num;j++)
	{
		printf("\n\n------------------CYCLE %d-----------------\n\n",j+1);

		/*these functions just transfers from i/p to o/p */
		
		if(t==0)//start of if t==0
		{
		if(strcmp(wb_ip.opcode,"HALT")==0)
			t=1;
		wb();
		mem();
/*
		if(mul2_ip.instr_n<alu_ip.instr_n)
		{
			mul2();
			if(mul1_ip.instr_n<alu_ip.instr_n)
			{
				mul1();
				alu();
			}
			else
			{
				alu();
				mul1();
			}
		}
		else
		{
			alu();
			mul2();
			mul1();
		}

*/
		
		div4();	
		div3();
		div2();
		div1();
		mul2();
		mul1();
		alu();

		decode();
		

		/*if(strlen(f_op.opcode)==0)
		{
			fetch();
		}*/
		fetch();


		/*THESE FUNCTIONS TRANSFER DATA FROM ONE STAGE TO ANOTHER */
		if(strlen(wb_ip.opcode)==0)
		{
			inputTOoutput(&m_op,&wb_ip);
		}
	/*
		if(strlen(m_ip.opcode)==0)
		{
			if(alu_op.instr_n<mul2_op.instr_n)
			{
				inputTOoutput(&alu_op,&m_ip);
			}
			else
			{
				inputTOoutput(&mul2_op,&m_ip);
			}
		}
	*/

		if(strlen(m_ip.opcode)==0)
		{

			//printf("\nIN ALU--->>>MEM IF\n");
			//printf("\nR[%d].status=%d\nR[%d].status=%d\n",alu_op.source_registers[0],R[alu_op.source_registers[0]].status,alu_op.source_registers[1],R[alu_op.source_registers[1]].status);
			if(div4_op.dest_register==div4_op.source_registers[0])
				R[div4_op.source_registers[0]].status=0;
			if(div4_op.dest_register==div4_op.source_registers[1])
				R[div4_op.source_registers[1]].status=0;


			if(mul2_op.dest_register==mul2_op.source_registers[0])
				R[mul2_op.source_registers[0]].status=0;
			if(mul2_op.dest_register==mul2_op.source_registers[1])
				R[mul2_op.source_registers[1]].status=0;


			if(alu_op.dest_register==alu_op.source_registers[0])
				R[alu_op.source_registers[0]].status=0;
			if(alu_op.dest_register==alu_op.source_registers[1])
				R[alu_op.source_registers[1]].status=0;


			//printf("\n%%%INST NUMBERS:\n");
			//printf("div4 instr= %d\nmul2 instr= %d\n",div4_op.instr_n,mul2_op.instr_n);
			/*if(strlen(div4_op.opcode)!=0 && R[div4_op.source_registers[0]].status==0 && R[div4_op.source_registers[1]].status==0)
			{
				inputTOoutput(&div4_op,&m_ip);
			}
			else if(strlen(mul2_op.opcode)!=0 && R[mul2_op.source_registers[0]].status==0 && R[mul2_op.source_registers[1]].status==0)
			{
				inputTOoutput(&mul2_op,&m_ip);
			}
			else if(strlen(alu_op.opcode)!=0 && R[alu_op.source_registers[0]].status==0 && R[alu_op.source_registers[1]].status==0)
			{
				printf("\nIN ALU->MEM IF-ELSE-IF\n");
				inputTOoutput(&alu_op,&m_ip);
			}
			else
			{
				printf("\nIN ALU->MEM IF-ELSE\n");
			}
			*/
			int d=0,m=0,a=0,set=0;
			if(strlen(div4_op.opcode)!=0)
				d=1;
			if(strlen(mul2_op.opcode)!=0)
				m=1;
			if(strlen(alu_op.opcode)!=0)
				a=1;

			if(strlen(div4_op.opcode)!=0 && set==0) //checking all conditions to pass div4 to memory stage
			{
				if(R[div4_op.source_registers[1]].status==0 && R[div4_op.source_registers[0]].status==0)
				{
					inputTOoutput(&div4_op,&m_ip);
					set=1;
				}
				else if(m==0 && a==0)
				{
					inputTOoutput(&div4_op,&m_ip);	
					set=1;
				}
				else if(m==1 && a==1)
				{
					if(div4_op.instr_n<mul2_op.instr_n && div4_op.instr_n<alu_op.instr_n)
					{
						inputTOoutput(&div4_op,&m_ip);
						set=1;	
					}
					else if(div4_op.instr_n<mul2_op.instr_n)
					{
						inputTOoutput(&div4_op,&m_ip);
						set=1;
					}
					else if(div4_op.instr_n<alu_op.instr_n)
					{
						inputTOoutput(&div4_op,&m_ip);
						set=1;
					}
					else
					{

					}
				}	//end of else if
				else if(m==1 && a==0)
				{
					if(div4_op.instr_n<mul2_op.instr_n)
					{
						inputTOoutput(&div4_op,&m_ip);
						set=1;
					}
				}
				else if(m==0 && a==1)
				{
					if(div4_op.instr_n<alu_op.instr_n)
					{
						inputTOoutput(&div4_op,&m_ip);
						set=1;
					}
				}
				else
				{

				}

			}
			
			if(strlen(mul2_op.opcode)!=0 && set==0)
			{
				if(R[mul2_op.source_registers[0]].status==0 && R[mul2_op.source_registers[1]].status==0)
				{
					inputTOoutput(&mul2_op,&m_ip);
					set=1;
				}
				else if(a==1)
				{
					if(mul2_op.instr_n<alu_op.instr_n)
					{
						inputTOoutput(&mul2_op,&m_ip);
						set=1;
					}
				}
				else if(a==0)
				{
					inputTOoutput(&mul2_op,&m_ip);
					set=1;
				}
			}
			if(strlen(alu_op.opcode)!=0 && R[alu_op.source_registers[0]].status==0 && R[alu_op.source_registers[1]].status==0 && set==0)
			{
				//printf("\nIN ALU->MEM IF-ELSE-IF\n");
				inputTOoutput(&alu_op,&m_ip);
				set=1;
			}
			else
			{
				//printf("\nIN ALU->MEM IF-ELSE\n");
			}


		
		
		} 

		if(strlen(mul2_ip.opcode)==0 && strlen(mul2_op.opcode)==0)
		{
			inputTOoutput(&mul1_op,&mul2_ip);
		}

		if(strlen(div4_ip.opcode)==0 && strlen(div4_op.opcode)==0)
		{
			inputTOoutput(&div3_op,&div4_ip);
		}

		if(strlen(div3_ip.opcode)==0 && strlen(div3_op.opcode)==0)
		{
			inputTOoutput(&div2_op,&div3_ip);
		}

		if(strlen(div2_ip.opcode)==0 && strlen(div2_op.opcode)==0)
		{
			inputTOoutput(&div1_op,&div2_ip);
		}


		//printf("\nin decode->alu\n");
		//printf("d_op.dest_register=%d",d_op.dest_register);
		//printf("\nR[%d].src_status=%d\nR[%d].src_status=%d\nR[%d].src_status=%d\n",d_op.dest_register,R[d_op.dest_register].src_status,d_op.source_registers[0],R[d_op.source_registers[0]].src_status,d_op.source_registers[1],R[d_op.source_registers[1]].src_status);
			
		if(strlen(alu_ip.opcode)==0 && strcmp(d_op.opcode,"MUL")!=0 && strcmp(d_op.opcode,"DIV")!=0 && strcmp(d_op.opcode,"HALT")!=0 && strlen(alu_op.opcode)==0 && R[d_op.dest_register].src_status==0 && R[d_op.source_registers[0]].src_status==0 && R[d_op.source_registers[1]].src_status==0)
		{
			//printf("\ninside if stmt decode->alu\n");
			//printf("\nR[%d].src_status=%d\nR[%d].src_status=%d\nR[%d].src_status=%d\n",d_op.dest_register,R[d_op.dest_register].src_status,d_op.source_registers[0],R[d_op.source_registers[0]].src_status,d_op.source_registers[1],R[d_op.source_registers[1]].src_status);
			if(strcmp(d_op.opcode,"BNZ")!=0 && strcmp(d_op.opcode,"BZ")!=0)
			{
				if(opDependency[d_op.dest_register]==0)
				{
					//printf("\n(((((((((((*******)))))))))))\n");
					R[d_op.dest_register].status=1; //it is to be unset in WB stage
					R[d_op.dest_register].src_status=1; //to be unset in EX stage
					inputTOoutput(&d_op,&alu_ip);
					//printf("\naluip opcode= %s\n",alu_ip.opcode);
				}
			}


			if(strcmp(d_op.opcode,"BNZ")==0 || strcmp(d_op.opcode,"BZ")==0)
			{
				//printf("\n********IN BNZ IF*******\n");
				/*
				if(strlen(m_ip.opcode)==0 && strlen(m_op.opcode)==0 && strlen(wb_ip.opcode)==0 && strlen(wb_op.opcode)==0 && strlen(mul1_ip.opcode)==0 && strlen(mul1_op.opcode)==0&& strlen(mul2_ip.opcode)==0&& strlen(mul2_op.opcode)==0)
				{
					//R[d_op.dest_register].status=1; 
					//R[d_op.dest_register].src_status=1; 
					inputTOoutput(&d_op,&alu_ip);
				}
				*/

				//it may have movc instr before which does not set zero flag
				if(strlen(div1_ip.opcode)==0 && strlen(div1_op.opcode)==0 && strlen(div2_ip.opcode)==0 && strlen(div2_op.opcode)==0 && strlen(div3_ip.opcode)==0 && strlen(div3_op.opcode)==0 && strlen(div4_ip.opcode)==0 && strlen(div4_op.opcode)==0 && strlen(mul1_ip.opcode)==0 && strlen(mul1_op.opcode)==0 && strlen(mul2_ip.opcode)==0 && strlen(mul2_op.opcode)==0 && strlen(alu_ip.opcode)==0 && strlen(alu_op.opcode)==0)
				{
					//printf("\n********IN BNZ  IF-IF*******\n");
					if(strlen(m_ip.opcode)!=0)
					{
						// && strlen(m_op.opcode)!=0
						//printf("\n********IN BNZ  IF-IF-IF*******\n");
						if(strcmp(m_ip.opcode,"ADD")==0 || strcmp(m_ip.opcode,"SUB")==0 || strcmp(m_ip.opcode,"MUL")==0 ||strcmp(m_ip.opcode,"DIV")==0)
						{
							// || strcmp(m_op.opcode,"ADD")==0 || strcmp(m_op.opcode,"SUB")==0 || strcmp(m_op.opcode,"MUL")==0 ||strcmp(m_op.opcode,"DIV")==0
							inputTOoutput(&d_op,&alu_ip);
						}
						if(strcmp(m_ip.opcode,"ADD")!=0 && strcmp(m_ip.opcode,"SUB")!=0 && strcmp(m_ip.opcode,"MUL")!=0 && strcmp(m_ip.opcode,"DIV")!=0)
						{
							if(strcmp(wb_ip.opcode,"ADD")!=0 && strcmp(wb_ip.opcode,"SUB")!=0 && strcmp(wb_ip.opcode,"MUL")!=0 && strcmp(wb_ip.opcode,"DIV")!=0)
							{
								//printf("\n********IN BNZ  IF-IF-IF-IF-IF*******\n");
								// || strcmp(m_op.opcode,"ADD")==0 || strcmp(m_op.opcode,"SUB")==0 || strcmp(m_op.opcode,"MUL")==0 ||strcmp(m_op.opcode,"DIV")==0
								inputTOoutput(&d_op,&alu_ip);
							}
						}
					}
					if(strlen(m_ip.opcode)==0 && strlen(m_op.opcode)==0 && strlen(wb_ip.opcode)==0 && strlen(wb_op.opcode)==0)
					{
						inputTOoutput(&d_op,&alu_ip);
					}
				}
				else if (strcmp(div1_ip.opcode,"DIV")!=0 && strcmp(div1_op.opcode,"DIV")!=0 && strcmp(div2_ip.opcode,"DIV")!=0 && strcmp(div2_op.opcode,"DIV")!=0 && strcmp(div3_ip.opcode,"DIV")!=0 && strcmp(div3_op.opcode,"DIV")!=0 && strcmp(div4_ip.opcode,"DIV")!=0 && strcmp(div4_op.opcode,"DIV")!=0 && strcmp(mul1_ip.opcode,"MUL")!=0 && strcmp(mul1_op.opcode,"MUL")!=0 && strcmp(mul2_ip.opcode,"MUL")!=0 && strcmp(mul2_op.opcode,"MUL")!=0 && strcmp(alu_ip.opcode,"ADD")!=0 && strcmp(alu_op.opcode,"ADD")!=0 && strcmp(alu_ip.opcode,"SUB")!=0 && strcmp(alu_op.opcode,"SUB")!=0)
				{
					//printf("\n********IN BNZ IF-ELSE IF*******\n");
					if(strlen(m_ip.opcode)!=0)// && strlen(m_op.opcode)!=0
					{
						//printf("\n^^^^^^^^^^IN ELSE -IF_IF\n");
						if(strcmp(m_ip.opcode,"ADD")==0 || strcmp(m_ip.opcode,"SUB")==0 || strcmp(m_ip.opcode,"MUL")==0 ||strcmp(m_ip.opcode,"DIV")==0 || strcmp(m_op.opcode,"ADD")==0 || strcmp(m_op.opcode,"SUB")==0 || strcmp(m_op.opcode,"MUL")==0 ||strcmp(m_op.opcode,"DIV")==0)
						{
							//printf("\n^^^^^^^^^^IN ELSE -IF_IF-IF\n");
							inputTOoutput(&d_op,&alu_ip);
						}

					}
					if(strlen(m_ip.opcode)==0 && strlen(m_op.opcode)==0)
					{
						if(strlen(wb_ip.opcode)==0 && strlen(wb_op.opcode)==0)
						{
							//printf("\n^^^^^^^^^^1st\n");
							inputTOoutput(&d_op,&alu_ip);
						}
						else if(strcmp(wb_ip.opcode,"ADD")!=0 && strcmp(wb_ip.opcode,"SUB")!=0 && strcmp(wb_ip.opcode,"MUL")!=0 && strcmp(wb_ip.opcode,"DIV")!=0)
						{
							//printf("\n^^^^^^^^^2nd\n");
							inputTOoutput(&d_op,&alu_ip);
						}
					}
					if(strcmp(m_ip.opcode,"ADD")!=0 && strcmp(m_ip.opcode,"SUB")!=0 && strcmp(m_ip.opcode,"MUL")!=0 && strcmp(m_ip.opcode,"DIV")!=0 && strcmp(m_op.opcode,"ADD")!=0 && strcmp(m_op.opcode,"SUB")!=0 && strcmp(m_op.opcode,"MUL")!=0 && strcmp(m_op.opcode,"DIV")!=0)
					{
						//printf("\n^^^^^^^^^^IN ELSE -IF_3rd IF\n");
						if(strcmp(wb_ip.opcode,"ADD")!=0 && strcmp(wb_ip.opcode,"SUB")!=0 && strcmp(wb_ip.opcode,"MUL")!=0 && strcmp(wb_ip.opcode,"DIV")!=0 && strcmp(wb_op.opcode,"ADD")==0 && strcmp(wb_op.opcode,"SUB")==0 && strcmp(wb_op.opcode,"MUL")==0 && strcmp(wb_op.opcode,"DIV")==0)
						{
							//printf("\n^^^^^^^^^^IN ELSE -IF_3rd IF-IF\n");
							//||strcmp(wb_op.opcode,"ADD")==0 || strcmp(wb_op.opcode,"SUB")==0 || strcmp(wb_op.opcode,"MUL")==0 || strcmp(wb_op.opcode,"DIV")==0
							inputTOoutput(&d_op,&alu_ip);
						}
					}

				}
				else
				{

				}

			}// end of if opcode= BNZ or BZ

		}
			
		if(strlen(mul1_ip.opcode)==0 && strcmp(d_op.opcode,"MUL")==0 && strlen(mul1_op.opcode)==0 && R[d_op.dest_register].src_status==0 && R[d_op.source_registers[0]].src_status==0 && R[d_op.source_registers[1]].src_status==0)
		{	
			if(opDependency[d_op.dest_register]==0)
			{
				R[d_op.dest_register].status=1;
				R[d_op.dest_register].src_status=1;
				inputTOoutput(&d_op,&mul1_ip);
			}
		}

		if((strlen(div1_ip.opcode)==0 && strcmp(d_op.opcode,"DIV")==0 && strlen(div1_op.opcode)==0 && R[d_op.dest_register].src_status==0 && R[d_op.source_registers[0]].src_status==0 && R[d_op.source_registers[1]].src_status==0) || (strlen(div1_ip.opcode)==0 && strcmp(d_op.opcode,"HALT")==0 && strlen(div1_op.opcode)==0))
		{	
			if(strcmp(d_op.opcode,"DIV")==0)
			{
				inputTOoutput(&d_op,&div1_ip);
			}
			else if(opDependency[d_op.dest_register]==0)
			{
				R[d_op.dest_register].status=1;
				R[d_op.dest_register].src_status=1;
				inputTOoutput(&d_op,&div1_ip);
			}
		}

		if(strlen(d_ip.opcode)==0 && strlen(d_op.opcode)==0)
		{
			//printf("\n**************IN FETCH->>>DECODE****\n");
			inputTOoutput(&f_op,&d_ip);
		}
			
		if(strlen(f_ip.opcode)==0 && instr_tracker!=1 && strlen(f_op.opcode)==0)
		{
			if(instr_loop==-1)
			{
				fetchinputTOoutput(&ii[instr_tracker-1],&f_ip);
			}
			else
			{
				instr_tracker=instr_loop;
				fetchinputTOoutput(&ii[instr_tracker-1],&f_ip);
				instr_loop=-1;
			}
		}

		
		}//end of if t==0
	} //end of for loop- cycles loop




	printf("\n......................\nREGISTER FILE:\n");
	for(int z=0;z<16;z++)
	{
		printf("R%d -> %d\n",z,R[z]);
	}
	fclose(fp);//file is closed
	return 0;
}

