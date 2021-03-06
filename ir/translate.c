
#include "translate.h"

list_node_InterCode* translate_Exp(struct Node* exp, char* *place) {
	Log("translate Exp: ");
	Assert(exp && (exp->type == TYPE_Exp), "Invalid expression: %s", type_list[exp->type]);
	list_node_InterCode* code = NULL;

	if(exp->fc->type == TYPE_INT) {				// Exp -> INT
		char *imm = (char*)malloc(12 * sizeof(char));
		imm[0] = '#';
		strcpy(imm + 1, exp->fc->val);
		*place = imm;
	} else if(exp->fc->type == TYPE_FLOAT) {	// Exp -> FLOAT
		Assert(0, "I can't translate float number");
	} else if(exp->fc->type == TYPE_ID) {
		if(exp->fc->ns == NULL) {						// Exp -> ID
			*place = exp->fc->val;
		} else if(exp->fc->ns->ns->type == TYPE_RP) {	// Exp -> ID LP RP
			*place = new_temp();
			if(strcmp(exp->fc->val, "read") == 0) {
				list_push_back_InterCode(&code, gen_code(IC_READ, *place, NULL, NULL, NULL));
			} else {
				list_push_back_InterCode(&code, gen_code(IC_ECALL, *place, exp->fc->val, NULL, NULL));
			}
		} else {				// Exp -> ID LP Args RP
			*place = new_temp();
			list_node_Item *args = NULL;
			list_merge_InterCode(&code, translate_Args(exp->fc->ns->ns, &args));
			if(strcmp(exp->fc->val, "write") == 0) {
				list_push_back_InterCode(&code, gen_code(IC_WRITE, (args->data).name, NULL, NULL, NULL));
				return code;
			}
			while(args != NULL) {
				list_push_back_InterCode(&code, gen_code(IC_ARG, (args->data).name, NULL, NULL, NULL));
				args = args->next;
			}
			list_push_back_InterCode(&code, gen_code(IC_ECALL, *place, exp->fc->val, NULL, NULL));
		}
	} else if(exp->fc->type == TYPE_LP) {		// Exp -> LP Exp RP
		code = translate_Exp(exp->fc->ns, place);
	} else if(exp->fc->type == TYPE_MINUS) {	// Exp -> MINUS Exp
		char *t1 = NULL;
		code = translate_Exp(exp->fc->ns, &t1);
		*place = new_temp();
		list_push_back_InterCode(&code, gen_code(IC_SUB, *place, "#0", t1, NULL));
	} else if(exp->fc->type == TYPE_NOT) {		// Exp -> NOT Exp
		char *label1 = new_label();
		char *label2 = new_label();
		*place = new_temp();
		list_push_back_InterCode(&code, gen_code(IC_ASSIGN, *place, "#0", NULL, NULL));
		list_merge_InterCode(&code, translate_Cond(exp, label1, label2));
		list_push_back_InterCode(&code, gen_code(IC_LABEL, label1, NULL, NULL, NULL));
		list_push_back_InterCode(&code, gen_code(IC_ASSIGN, *place, "#1", NULL, NULL));
		list_push_back_InterCode(&code, gen_code(IC_LABEL, label2, NULL, NULL, NULL));
	} else {
		Assert(exp->fc->type == TYPE_Exp, "Invalid expression");
		if(exp->fc->ns->type == TYPE_ASSIGNOP) {		// Exp ASSIGNOP Exp 
			char *t1 = NULL;
			char *t2 = NULL;
			list_merge_InterCode(&code, translate_Exp(exp->fc, &t1));
			list_merge_InterCode(&code, translate_Exp(exp->fc->ns->ns, &t2));
			list_push_back_InterCode(&code, gen_code(IC_ASSIGN, t1, t2, NULL, NULL));
			*place = t1;
			//*place = new_temp();
			//list_push_back_InterCode(&code, gen_code(IC_ASSIGN, *place, t1, NULL, NULL));
		} else if(exp->fc->ns->type == TYPE_PLUS) {		// Exp PLUS Exp 
			char *t1 = NULL;
			char *t2 = NULL;
			list_merge_InterCode(&code, translate_Exp(exp->fc, &t1));
			list_merge_InterCode(&code, translate_Exp(exp->fc->ns->ns, &t2));
			*place = new_temp();
			list_push_back_InterCode(&code, gen_code(IC_ADD, *place, t1, t2, NULL));
		} else if(exp->fc->ns->type == TYPE_MINUS) {		// Exp MINUS Exp 
			char *t1 = NULL;
			char *t2 = NULL;
			list_merge_InterCode(&code, translate_Exp(exp->fc, &t1));
			list_merge_InterCode(&code, translate_Exp(exp->fc->ns->ns, &t2));
			*place = new_temp();
			list_push_back_InterCode(&code, gen_code(IC_SUB, *place, t1, t2, NULL));
		} else if(exp->fc->ns->type == TYPE_STAR) {		// Exp STAR Exp 
			char *t1 = NULL;
			char *t2 = NULL;
			list_merge_InterCode(&code, translate_Exp(exp->fc, &t1));
			list_merge_InterCode(&code, translate_Exp(exp->fc->ns->ns, &t2));
			*place = new_temp();
			list_push_back_InterCode(&code, gen_code(IC_MUL, *place, t1, t2, NULL));
		} else if(exp->fc->ns->type == TYPE_DIV) {		// Exp DIV Exp 
			char *t1 = NULL;
			char *t2 = NULL;
			list_merge_InterCode(&code, translate_Exp(exp->fc, &t1));
			list_merge_InterCode(&code, translate_Exp(exp->fc->ns->ns, &t2));
			*place = new_temp();
			list_push_back_InterCode(&code, gen_code(IC_DIV, *place, t1, t2, NULL));
		} else if((exp->fc->ns->type == TYPE_AND) ||	// Exp AND Exp 
				(exp->fc->ns->type == TYPE_OR) ||		// Exp OR Exp 
				(exp->fc->ns->type == TYPE_RELOP)) {	// Exp RELOP Exp 
			char *label1 = new_label();
			char *label2 = new_label();
			*place = new_temp();
			list_push_back_InterCode(&code, gen_code(IC_ASSIGN, *place, "#0", NULL, NULL));
			list_merge_InterCode(&code, translate_Cond(exp, label1, label2));
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label1, NULL, NULL, NULL));
			list_push_back_InterCode(&code, gen_code(IC_ASSIGN, *place, "#1", NULL, NULL));
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label2, NULL, NULL, NULL));
		} else if(exp->fc->ns->type == TYPE_LB) {		// Exp LB Exp RB
			char *t1 = NULL;
			char *t2 = NULL;
			char *t3 = new_temp();
			if(exp->fc->fc->type != TYPE_ID)
				list_merge_InterCode(&code, translate_Exp(exp->fc, &t1));
			else
				t1 = "#0";

			list_merge_InterCode(&code, translate_Exp(exp->fc->ns->ns, &t2));
			list_push_back_InterCode(&code, gen_code(IC_ADD, t3, t1, t2, NULL));
			if(exp->vt_syn->kind == T_ARRAY) {
				char *imm = (char*)malloc(12 * sizeof(char));
				sprintf(imm, "#%d", (exp->vt_syn->array).sz);
				*place = new_temp();
				list_push_back_InterCode(&code, gen_code(IC_MUL, *place, t3, imm, NULL));
			} else {
				char tmp[12];
				*place = new_temp();
				strcpy(tmp, *place);
				strcpy(*place + 1, tmp);
				**place = '*';
				list_push_back_InterCode(&code, gen_code(IC_MUL, *place + 1, t3, "#4", NULL));
				struct Node *tn = exp->fc;
				while(tn->type != TYPE_ID) tn = tn->fc;
				list_push_back_InterCode(&code, gen_code(IC_ADD, *place + 1, tn->val, *place + 1, NULL));
			}
		} else if(exp->fc->ns->type == TYPE_DOT) {		// Exp DOT ID
			Assert(exp->fc->fc->type == TYPE_ID, "Too complex expression!");
			Assert(exp->fc->vt_syn->kind == T_STRUCTURE, "Invalid DOT usage.");
			Assert(exp->fc->ns->ns->type == TYPE_ID, "Too complex expression!");

			int offset = 0;
			char *name = exp->fc->ns->ns->val;
			list_node_Item *h = (exp->fc->vt_syn->structure).field;
			while(h != NULL) {
				if(strcmp((h->data).name, name) == 0) break;
				offset += vt_size((h->data).type);
				h = h->next;
			}
			assert(h);
			char *imm = (char*)malloc(12 * sizeof(char));
			sprintf(imm, "#%d", offset);
			char tmp[24];
			*place = new_temp();
			strcpy(tmp, *place);
			strcpy(*place + 1, tmp);
			**place = '*';
			list_push_back_InterCode(&code, gen_code(IC_ADD, *place + 1, exp->fc->fc->val, imm, NULL));
		} else {
			Assert(0, "Invalid expression: %s", type_list[exp->type]);
		}
	}

	return code;
}

list_node_InterCode* translate_Stmt(struct Node* stmt) {
	Log("translate Stmt: ");
	Assert(stmt && (stmt->type == TYPE_Stmt), "Invalid Stmt: %s", type_list[stmt->type]);
	list_node_InterCode *code = NULL;

	if(stmt->fc->type == TYPE_Exp) {				// Exp SEMI
		char *t1 = NULL;
		code = translate_Exp(stmt->fc, &t1);
	} else if(stmt->fc->type == TYPE_CompSt) {		// CompSt
		code = translate_CompSt(stmt->fc);
	} else if(stmt->fc->type == TYPE_RETURN) {		// RETURN Exp SEMI
		char *t1 = NULL;
		code = translate_Exp(stmt->fc->ns, &t1);
		list_push_back_InterCode(&code, gen_code(IC_RET, t1, NULL, NULL, NULL));
	} else if(stmt->fc->type == TYPE_IF) {
		if(stmt->fc->ns->ns->ns->ns->ns == NULL) {			// IF LP Exp RP Stmt
			char *label1 = new_label();
			char *label2 = new_label();
			code = translate_Cond(stmt->fc->ns->ns, label1, label2);
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label1, NULL, NULL, NULL));
			list_merge_InterCode(&code, translate_Stmt(stmt->fc->ns->ns->ns->ns));
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label2, NULL, NULL, NULL));
		} else {		// IF LP Exp RP Stmt ELSE Stmt	
			char *label1 = new_label();
			char *label2 = new_label();
			char *label3 = new_label();
			code = translate_Cond(stmt->fc->ns->ns, label1, label2);
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label1, NULL, NULL, NULL));
			list_merge_InterCode(&code, translate_Stmt(stmt->fc->ns->ns->ns->ns));
			list_push_back_InterCode(&code, gen_code(IC_GOTO, label3, NULL, NULL, NULL));
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label2, NULL, NULL, NULL));
			list_merge_InterCode(&code, translate_Stmt(stmt->fc->ns->ns->ns->ns->ns->ns));
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label3, NULL, NULL, NULL));
		}
	} else if(stmt->fc->type == TYPE_WHILE) {		// WHILE LP Exp RP Stmt
		char *label1 = new_label();
		char *label2 = new_label();
		char *label3 = new_label();
		list_push_back_InterCode(&code, gen_code(IC_LABEL, label1, NULL, NULL, NULL));
		list_merge_InterCode(&code, translate_Cond(stmt->fc->ns->ns, label2, label3));
		list_push_back_InterCode(&code, gen_code(IC_LABEL, label2, NULL, NULL, NULL));
		list_merge_InterCode(&code, translate_Stmt(stmt->fc->ns->ns->ns->ns));
		list_push_back_InterCode(&code, gen_code(IC_GOTO, label1, NULL, NULL, NULL));
		list_push_back_InterCode(&code, gen_code(IC_LABEL, label3, NULL, NULL, NULL));
	} else {
		Assert(0, "Invalid statement: %s", type_list[stmt->type]);
	}

	return code;
}

list_node_InterCode* translate_Cond(struct Node* cond, char *label_true, char *label_false) {
	Log("translate Cond: ");
	Assert(cond && (cond->type == TYPE_Exp), "Invalid condtion: %s", type_list[cond->type]);
	list_node_InterCode* code = NULL;

	if(cond->fc->type == TYPE_Exp) {
		if(cond->fc->ns->type == TYPE_RELOP) {
			char *t1 = NULL;
			char *t2 = NULL;
			list_merge_InterCode(&code, translate_Exp(cond->fc, &t1));
			list_merge_InterCode(&code, translate_Exp(cond->fc->ns->ns, &t2));
			list_push_back_InterCode(&code, gen_code(IC_COND, label_true, t1, t2, cond->fc->ns->val));
			list_push_back_InterCode(&code, gen_code(IC_GOTO, label_false, NULL, NULL, NULL));
		} else if(cond->fc->ns->type == TYPE_AND) {
			char *label1 = new_label();
			list_merge_InterCode(&code, translate_Cond(cond->fc, label1, label_false));
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label1, NULL, NULL, NULL));
			list_merge_InterCode(&code, translate_Cond(cond->fc->ns->ns, label_true, label_false));
		} else if(cond->fc->ns->type == TYPE_OR) {
			char *label1 = new_label();
			list_merge_InterCode(&code, translate_Cond(cond->fc, label_true, label1));
			list_push_back_InterCode(&code, gen_code(IC_LABEL, label1, NULL, NULL, NULL));
			list_merge_InterCode(&code, translate_Cond(cond->fc->ns->ns, label_true, label_false));
		}
	} else if(cond->fc->type == TYPE_NOT) {
		list_merge_InterCode(&code, translate_Cond(cond->fc->ns, label_false, label_true));
	} else {
		char *t1 = NULL;
		list_merge_InterCode(&code, translate_Exp(cond, &t1));
		list_push_back_InterCode(&code, gen_code(IC_COND, label_true, t1, "#0", "!="));
		list_push_back_InterCode(&code, gen_code(IC_GOTO, label_false, NULL, NULL, NULL));
	}

	return code;
}

list_node_InterCode* translate_Dec(struct Node* dec) {
	Log("translate Dec: ");
	Assert(dec && (dec->type == TYPE_Dec), "Invalid Dec: %s", type_list[dec->type]);
	list_node_InterCode *code = NULL;

	// Dec -> VarDec
	// Dec -> VarDec ASSIGNOP Exp
	list_merge_InterCode(&code, translate_VarDec(dec->fc));
	if(dec->fc->ns != NULL) {
		char *t1 = NULL;
		list_merge_InterCode(&code, translate_Exp(dec->fc->ns->ns, &t1));
		struct Node *tmp = dec->fc->fc;
		while(tmp->type != TYPE_ID)
			tmp = tmp->fc;

		assert(tmp != NULL);
		list_push_back_InterCode(&code, gen_code(IC_ASSIGN, tmp->val, t1, NULL, NULL));
	}

	return code;
}

list_node_InterCode* translate_DecList(struct Node* list) {
	Log("translate DecList: ");
	Assert(list && (list->type == TYPE_DecList), "Invalid DecList: %s", type_list[list->type]);
	list_node_InterCode *code = NULL;

	// DecList -> Dec
	// DecList -> Dec COMMA DecList
	list_merge_InterCode(&code, translate_Dec(list->fc));
	if(list->fc->ns != NULL)
		list_merge_InterCode(&code, translate_DecList(list->fc->ns->ns));

	return code;
}

list_node_InterCode* translate_Def(struct Node* def) {
	Log("translate Def: ");
	Assert(def && (def->type == TYPE_Def), "Invalid Def: %s", type_list[def->type]);
	list_node_InterCode *code = NULL;

	// Def -> Specifier DecList SEMI
	list_merge_InterCode(&code, translate_DecList(def->fc->ns));

	return code;
}

list_node_InterCode* translate_DefList(struct Node* list) {
	Log("translate DefList: ");
	Assert(list && (list->type == TYPE_DefList), "Invalid DefList: %s", type_list[list->type]);
	list_node_InterCode *code = NULL;

	// DefList -> NULL
	if(list->lineno == -1)	return code;

	// DefList -> Def DefList
	list_merge_InterCode(&code, translate_Def(list->fc));
	list_merge_InterCode(&code, translate_DefList(list->fc->ns));

	return code;
}

list_node_InterCode* translate_StmtList(struct Node* list) {
	Log("translate StmtList: ");
	Assert(list && (list->type == TYPE_StmtList), "Invalid StmtList: %s", type_list[list->type]);
	list_node_InterCode *code = NULL;

	// StmtList -> NULL
	if(list->lineno == -1) return code;

	// StmtList -> Stmt StmtList
	list_merge_InterCode(&code, translate_Stmt(list->fc));
	list_merge_InterCode(&code, translate_StmtList(list->fc->ns));

	return code;
}

list_node_InterCode* translate_CompSt(struct Node* st) {
	Log("translate CompSt: ");
	Assert(st && (st->type == TYPE_CompSt), "Invalid CompSt: %s", type_list[st->type]);
	list_node_InterCode* code = NULL;

	// CompSt -> LC DefList StmtList RC
	list_merge_InterCode(&code, translate_DefList(st->fc->ns));
	list_merge_InterCode(&code, translate_StmtList(st->fc->ns->ns));

	return code;
}

list_node_InterCode* translate_Args(struct Node* args, list_node_Item **arg_list) {
	Log("translate Args: ");
	Assert(args && (args->type == TYPE_Args), "Invalid Args: %s", type_list[args->type]);
	list_node_InterCode *code = NULL;

	// Exp [COMMA, Args]
	char *t1 = NULL;
	list_merge_InterCode(&code, translate_Exp(args->fc, &t1));
	Item i;
	strcpy(i.name, t1);
	i.type = args->fc->vt_syn;
	list_push_front_Item(arg_list, &i);
	if(args->fc->ns != NULL)		// Exp COMMA Args
		list_merge_InterCode(&code, translate_Args(args->fc->ns->ns, arg_list));

	return code;
}


list_node_InterCode* translate_FunDec(struct Node* fun) {
	Log("translate FunDec: ");
	Assert(fun && (fun->type == TYPE_FunDec), "Invalid FunDec: %s", type_list[fun->type]);
	list_node_InterCode *code = NULL;

	// FunDec -> ID LP RP
	// FunDec -> ID LP VarList RP
	list_push_back_InterCode(&code, gen_code(IC_FUNC, fun->fc->val, NULL, NULL, NULL));

	list_node_Item *args = NULL;
	if(fun->fc->ns->ns->type == TYPE_VarList)
		args = fun->fc->ns->ns->vt_list;
	while(args != NULL) {
		list_push_back_InterCode(&code, gen_code(IC_PARAM, (args->data).name, NULL, NULL, NULL));
		args = args->next;
	}

	return code;
}

list_node_InterCode* translate_VarDec(struct Node* var) {
	Log("translate VarDec: ");
	Assert(var && (var->type == TYPE_VarDec), "Invalid VarDec: %s", type_list[var->type]);
	list_node_InterCode *code = NULL;

	// VarDec -> ID
	// VarDec -> VarDec LB INT RB
	struct Node *tmp = var->fc;
	while(tmp->type != TYPE_ID)
		tmp = tmp->fc;

	int sz = vt_size(tmp->vt_syn);
	if(sz > 4) {
		char *szstr = (char*)malloc(12 * sizeof(char));
		sprintf(szstr, "%d", sz);
		list_push_back_InterCode(&code, gen_code(IC_DEC, tmp->val, szstr, NULL, NULL));
	}

	return code;
}

list_node_InterCode* translate_ExtDecList(struct Node* list) {
	Log("translate ExtDecList: ");
	Assert(list && (list->type == TYPE_ExtDecList), "Invalid ExtDecList: %s", type_list[list->type]);
	list_node_InterCode *code = NULL;

	if(list->fc->ns == NULL) {		// ExtDecList -> VarDec
		list_merge_InterCode(&code, translate_VarDec(list->fc));
	} else {		// ExtDecList -> VarDec COMMA ExtDecList
		list_merge_InterCode(&code, translate_VarDec(list->fc));
		list_merge_InterCode(&code, translate_ExtDecList(list->fc->ns->ns));
	}

	return code;
}

list_node_InterCode* translate_ExtDef(struct Node* extdef) {
	Log("translate ExtDef: ");
	Assert(extdef && (extdef->type == TYPE_ExtDef), "Invalid ExtDef: %s", type_list[extdef->type]);
	list_node_InterCode *code = NULL;

	if(extdef->fc->ns->type == TYPE_SEMI) {						// ExtDef -> Specifier SEMI
	} else if(extdef->fc->ns->type == TYPE_ExtDecList) {		// ExtDef -> Specifier ExtDecList SEMI
		list_merge_InterCode(&code, translate_ExtDecList(extdef->fc->ns));
	} else if(extdef->fc->ns->type == TYPE_FunDec) {
		if(extdef->fc->ns->ns->type == TYPE_SEMI) {				// ExtDef -> Specifier FunDec SEMI
		} else if(extdef->fc->ns->ns->type == TYPE_CompSt) {	// ExtDef -> Specifier FunDec CompSt
			list_merge_InterCode(&code, translate_FunDec(extdef->fc->ns));
			list_merge_InterCode(&code, translate_CompSt(extdef->fc->ns->ns));
		}
	}

	return code;
}

list_node_InterCode* translate_ExtDefList(struct Node* list) {
	Log("translate ExtDefList: ");
	Assert(list && (list->type == TYPE_ExtDefList), "Invalid ExtDefList: %s", type_list[list->type]);
	list_node_InterCode *code = NULL;

	// ExtDefList -> NULL
	if(list->lineno == -1)	return code;

	// ExtDefList -> ExtDef ExtDefList
	list_merge_InterCode(&code, translate_ExtDef(list->fc));
	list_merge_InterCode(&code, translate_ExtDefList(list->fc->ns));

	return code;
}

list_node_InterCode* translate_Program(struct Node* prog) {
	Log("translate Program: ");
	Assert(prog && (prog->type == TYPE_Program), "Invalid Program: %s", type_list[prog->type]);
	list_node_InterCode *code = NULL;

	// Program -> ExtDefList
	list_merge_InterCode(&code, translate_ExtDefList(prog->fc));

	return code;
}
