#include "header.h"

// Code part

extern unordered_map<SENTENCE_ID_TYPE, SentenceDNF> sentenceStore;
extern unordered_map<string, PRED_ID_TYPE> predictStore;
extern vector<SENTENCE_ID_TYPE> set_support;
extern vector<SENTENCE_ID_TYPE> set_aux;
extern Indexing myIndex;
extern IdGenerator<SENTENCE_ID_TYPE> sentenceId_generator;
extern IdGenerator<PRED_ID_TYPE> predictionId_generator;
extern IdGenerator<UNIV_ID_TYPE> universeId_generator;

void  Indexing::addSentence (SENTENCE_ID_TYPE sentenceId) {
	vector<Literal> literals = sentenceStore[sentenceId].getLiterals();
	for(int i = 0; i < literals.size(); i++) {
		Literal l = literals[i];
		if(l.getTrueOrNegated()) map[l.getPredictId()].addToTrueList(sentenceId);
		else map[l.getPredictId()].addToFalseList(sentenceId);
	}
}

string Indexing::stringify () {
	stringstream ss;
	for(auto lt = map.begin(); lt != map.end(); lt++) {
		string predName;
		PRED_ID_TYPE id = lt->first;
		TrueFalseLists list = lt->second;
		for(auto ltttt = predictStore.begin(); ltttt != predictStore.end(); ltttt++) {
			if(ltttt->second == id)
				predName = ltttt->first;
		}
		ss<<"---------------------------------------------------"<<endl;
		ss<<predName<<"\t True:\t";
		set<SENTENCE_ID_TYPE> trues = list.getTrueList();
		for(auto i = trues.begin(); i != trues.end(); i++) {
			ss<<sentenceStore[*i].stringify()<<",\t";
		}
		ss<<endl;
		ss<<predName<<"\t False:\t";
		set<SENTENCE_ID_TYPE> falses = list.getFalseList();
		for(auto i = falses.begin(); i != falses.end(); i++) {
			ss<<sentenceStore[*i].stringify()<<",\t";
		}
		ss<<endl;
	}
	return ss.str();
}

string Literal:: stringify() {
	stringstream ss;
	if(!noNegation) ss <<"~";
	for(auto lt = predictStore.begin(); lt != predictStore.end(); lt++) {
		if(lt->second == predict)
		ss<< lt->first;
	}
	ss<<"(";
	for (int i = 0; i < paramList.size(); ++i)
	{
		ss<< paramList[i].stringify();
		if(i != paramList.size() - 1) ss<<",";
	}
	ss<<")";
	return ss.str();
}

string SentenceFOL:: stringify() {
	stringstream ss;
	if(single) {
		if(negated) ss<<"~";
		ss << singleLiteral.stringify();
	}
	else {
		if(negated) {
			ss << " ~(";
			ss << op1->stringify();
			if(operat == IMPLY) ss<<" => ";
			if(operat == AND) ss<<" & ";
			if(operat == OR) ss<<" | ";
			ss << op2->stringify();
			ss <<") ";
		}
		else {
			ss << " (";
			ss << op1->stringify();
			if(operat == IMPLY) ss<<" => ";
			if(operat == AND) ss<<" & ";
			if(operat == OR) ss<<" | ";
			ss << op2->stringify();
			ss << " )";
		}
	}
	return ss.str();
}

string SentenceDNF::stringify() {
	stringstream ss;
	int univeral_id_local_represent = 0;
	unordered_map<int, int> find_local_represent;
	for(int i = 0; i < list.size(); i++) {
		Literal& lit = list[i];
		if(!lit.getTrueOrNegated()) ss <<"~";
		for(auto lt = predictStore.begin(); lt != predictStore.end(); lt++) {
			if(lt->second == lit.getPredictId())
				ss<< lt->first;
		}
		ss<<"(";
		for (int j = 0; j < lit.paramList.size(); ++j)
		{
			if(lit.paramList[j].isUniverse) {
				if(find_local_represent.find(lit.paramList[j].universeId) == find_local_represent.end()) {
						find_local_represent[lit.paramList[j].universeId] = univeral_id_local_represent;
						ss <<"u"<<univeral_id_local_represent++;
				}
				else {
						ss <<"u"<<find_local_represent[lit.paramList[j].universeId];
				}
			}
			else ss<<lit.paramList[j].constNname;
			if(j != lit.paramList.size() - 1) ss<<",";
		}
		ss<<")";

		if(i != list.size()-1) ss<<" | ";
	}
	return ss.str();
}

void SentenceFOL::generalToCNF() {
	if(!single) {
			if(operat == OR) {
				if(!op1->isSingle() && op1->operat == AND) {
					//(t1.op1 & t1.op2) | (t2.op1 |/& .....)
					SentenceFOL *t1 = op1;
					SentenceFOL *t2 = op2;
					// TODO: FIX THE MEMORY LEAK
					op1 = new SentenceFOL(OR, *(t1->op1), *t2);
					op2 = new SentenceFOL(OR, *(t1->op2), *t2);
					operat = AND;
				}
				else if(!op2->isSingle() && op2->operat == AND) {
					// (t1.op1 |/& .....) | (t2.op1 & t2.op2)
					SentenceFOL *t1 = op1;
					SentenceFOL *t2 = op2;
					// TODO: FIX THE MEMORY LEAK
					op1 = new SentenceFOL(OR, *t1, *(t2->op1));
					op2 = new SentenceFOL(OR, *t1, *(t2->op2));
					operat = AND;
				}

			}
			// Should it be out of the if(operat == OR) ?
			if(op1) op1->generalToCNF();
			if(op2) op2->generalToCNF();
	}
}

void SentenceFOL::eliminateImplication() {
	// Implication Elimination
	if(!single) {
		if(operat == IMPLY) {
				operat = OR;
				op1->negate();
		}
		op1->eliminateImplication();
		op2->eliminateImplication();
	}
}

void SentenceFOL::walkInNegation() {
	if(negated) {
		if(!single) {
			negated = false;
			if(operat == OR) operat = AND;
			else if(operat == AND) operat = OR;
			op1->negate();
			op2->negate();
		}
	}
	if(!single) {
		op1->walkInNegation();
		op2->walkInNegation();
	}
}

bool SentenceFOL::addToKB(SET_TYPE set) {
	eliminateImplication();
	walkInNegation();
	generalToCNF();
	putCNFIntoSentenceStore(set);
	return true;
}

// (A|B) & (C|D) & (E|F|G) & ....
void SentenceFOL::putCNFIntoSentenceStore(SET_TYPE set) {
	if(single) {
		SENTENCE_ID_TYPE id = sentenceId_generator.getNext();
		sentenceStore[id] = getDNFByFOL();
		sentenceStore[id].setSet(set);
		myIndex.addSentence(id);
		if(set == SUPPORT_SET) set_support.push_back(id);
		else if(set == AUX_SET) set_aux.push_back(id);
	}
	else {
		if(operat == AND) {
			op1->putCNFIntoSentenceStore(set);
			op2->putCNFIntoSentenceStore(set);
		}
		else {
			SENTENCE_ID_TYPE id = sentenceId_generator.getNext();
			sentenceStore[id] = getDNFByFOL();
			sentenceStore[id].setSet(set);
			myIndex.addSentence(id);
			if(set == SUPPORT_SET) set_support.push_back(id);
			else if(set == AUX_SET) set_aux.push_back(id);
		}
	}
}

// A|B|C|D|E
SentenceDNF SentenceFOL::getDNFByFOL() {
	SentenceDNF res;
	if(single) {
		res.add(singleLiteral);
	}
	else if(operat == OR){
		op1->getDNFByFOL(res);
		op2->getDNFByFOL(res);
	}
	return res;
}

void SentenceFOL::getDNFByFOL(SentenceDNF& dnf) {
	if(single) {
		dnf.add(singleLiteral);
	}
	else if(operat == OR){
		op1->getDNFByFOL(dnf);
		op2->getDNFByFOL(dnf);
	}
}

int getPredIdByName(string name) {
	if(predictStore.find(name) == predictStore.end()) {
		predictStore[name] = predictionId_generator.getNext();
	}
	return predictStore[name];
}

bool find_a_substitution(
	vector<Element>& elems1, vector<Element>& elems2,
	unordered_map<Element, Element>& replace1,
	unordered_map<Element, Element>& replace2 ) {

		for(int j = 0; j < elems1.size(); j++) {
			Element& elem1 = elems1[j];
			Element& elem2 = elems2[j];
			if( (elem1.isUniverse && !elem2.isUniverse) || (elem2.isUniverse && !elem1.isUniverse)) {
				if(elem1.isUniverse) replace1[elem1] = elem2;
				else replace2[elem2] = elem1;
			}
			else if(!elem1.isUniverse && !elem2.isUniverse) {
				// Conflict! skip this literal
				// As we cannot assign val to a constant
				if(elem1.constNname != elem2.constNname) return false;
			}
			else {
				replace1[elem1] = elem2;
			}
		}
		return true;
}

void apply_a_substitution(vector<Literal>& list1, vector<Literal>& list2,
	unordered_map<Element, Element>& replace1,
	unordered_map<Element, Element>& replace2 ) {
		// Each predicate
		for(int j = 0; j < list1.size(); j++) {
			vector<Element>& elms = list1[j].getElements();
			// Each literal
			for(int k = 0; k < elms.size(); k++) {
				if(replace1.find(elms[k]) != replace1.end())
					elms[k] = replace1[elms[k]];
			}
		}

		for(int j = 0; j < list2.size(); j++) {
			vector<Element>& elms = list2[j].getElements();
			// Each literal
			for(int k = 0; k < elms.size(); k++) {
				if(replace2.find(elms[k]) != replace2.end()) {
					elms[k] = replace2[elms[k]];
				}
			}
		}
}

//Eliminate exactly same literals F(u1,A,B,u2) with F(u1,A,B,u2)
void EliminateExactlySameLiterals(vector<Literal>& list1, vector<Literal>& list2) {
	set<string> s1_literals;
	if(list2.size() == 0 || list1.size() == 0) return;
	for(int i = 0 ; i < list1.size(); i++ ) {
			s1_literals.insert(list1[i].stringify());
	}
	for(int i = 0 ; i < list2.size();) {
			if(s1_literals.find(list2[i].stringify()) != s1_literals.end() ) {
				//cout <<"found" <<endl;
				list2.erase(list2.begin() + i);
			}
			else i++;
	}
}

bool duplicateWithAncestors(string sentence, SENTENCE_ID_TYPE parent) {
	if(sentence == sentenceStore[parent].stringify()) {
		return true;
	}
	for(auto ll = sentenceStore[parent].parents.begin(); ll != sentenceStore[parent].parents.end(); ll++) {
		if(duplicateWithAncestors(sentence, *ll))
			return true;
	}
	return false;
}



// 0-cannot resolve
// 1-resolved successfully
// 2-resolve into empty set

int resolution_and_put_result_into_support_set(SENTENCE_ID_TYPE id1, long p1, SENTENCE_ID_TYPE id2, SET_TYPE set_id, vector<SENTENCE_ID_TYPE>& set_to_put) {
	SentenceDNF s1 = sentenceStore[id1];
	SentenceDNF s2 = sentenceStore[id2];
	vector<Literal> list1 = s1.getLiterals();
	Literal literal1 = list1[p1];
	vector<Literal> list2 = s2.getLiterals();
	long list2_matched_position = -1;

	// Unify and Resolution
	for(int i = 0; i < list2.size(); i++) {
		// Find first substitutable literal
		if(list2[i].getPredictId() == literal1.getPredictId() && list2[i].getTrueOrNegated()!=literal1.getTrueOrNegated()) {
			unordered_map<Element, Element> replace1;
			unordered_map<Element, Element> replace2;
			vector<Element>& elems1 = literal1.getElements();
			vector<Element>& elems2 = list2[i].getElements();

			// Find substitution
			if(find_a_substitution(elems1, elems2, replace1, replace2)) list2_matched_position = i;

			// Apply substitution
			apply_a_substitution(list1, list2, replace1, replace2);

		}
	}

	if(list2_matched_position != -1) {

		list1.erase(list1.begin() + p1);
		list2.erase(list2.begin() + list2_matched_position);

		EliminateExactlySameLiterals(list1, list2);

		list1.insert( list1.end(), list2.begin(), list2.end() );
		if(list1.size() == 0) return 2;
		SentenceDNF newSentence(set_id);
		for(int i = 0; i < list1.size(); i++) {
			newSentence.add(list1[i]);
		}

		for(auto lt = sentenceStore.begin(); lt != sentenceStore.end(); lt++) {
			if(newSentence.stringify() == lt->second.stringify()) {
				//cout <<"Already in sentenceStore"<<endl;
				return 0;
			}
		}

		newSentence.setParent(id1);
		newSentence.setParent(id2);

		if(duplicateWithAncestors(newSentence.stringify(), id1) || duplicateWithAncestors(newSentence.stringify(), id2)) {
			return 0;
		}

		SENTENCE_ID_TYPE id = sentenceId_generator.getNext();
		sentenceStore[id] = newSentence;
		myIndex.addSentence(id);
		set_to_put.push_back(id);
		cout<<"Get  " << id <<" by Resolve  "<<id1 << " with  "<< id2<<endl;
		return 1;
	}

	return 0;
}