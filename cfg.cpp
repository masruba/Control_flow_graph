#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <fstream>
#include <map>
#include <algorithm>
#include <assert.h>
#include <set>
#include <stack>
using namespace std;

// Each line is at most 254 characters long
#define SIZE 256

class CFG{
	class BasicBlock{
	public:
		int startLine;
		int endLine;

		BasicBlock(int s = -1, int e = -1){
			startLine = s;
			endLine = e;
		}
		bool operator< (const BasicBlock &a) const {
			return (startLine < a.startLine);
		}
	};

	int nVars;
	int nInstrunctions;
	int endInstructionLineNo;
	int lineNo;
	int nBasicBlock;

	vector<string> varLines;

	class Instr{
		public:
		string src1;
		string src2;
		string dst;
		string label;
		string predicate;
		string ins;
		// 1 = unconditional branch
		// 2 = conditional
		// is i-th instruction branch?
		int instructionType;

		Instr(){
			instructionType = -1;
		}

		Instr(string s){
			ins = s;
		}
	};
	vector<Instr> instructions;

	map<string, int> labelToLineNoMap;
	vector<string> pendingLabelLineUpdateMap;
	vector<int> pendingLabelInstructionLineNo;

	set<int> leaderLines;
	vector<BasicBlock> block;

	// Adjacency Matrix
	int** adjMat;

	// Parent list of all nodes
	vector<int>* parents;

	// Dominator Sets of all nodes
	vector<int>* domSet;

	// For loop construction
	stack<int> S;

	// Temporary Loop
	vector<int> loop;

	// All loops
	vector<int>* loops;

	// Set of back edges
	class Edge{
		public:
		int x;
		int y;
		Edge(int a, int b){
			x = a;
			y = b;
		}
	};
	vector<Edge> backEdges;

public:
	CFG(){
		lineNo = 0;
		nVars = 0;
		nInstrunctions = 0;
		endInstructionLineNo = -1;
		nBasicBlock = 0;

		leaderLines.clear();
		block.clear();
	}

	bool leadersIsEmpty(){
		if(leaderLines.size() == 0)
			return true;
		return false;
	}

	void addLeader(int lineNo){
		leaderLines.insert(lineNo);
	}

	void print(vector<int> v, int offset = 0, string prefix=""){
		vector<int>::iterator it;
		for(it = v.begin(); it!=v.end(); it++){
			if(!prefix.empty())	cout << prefix;
			printf("%d ", *it+offset);
		}
		printf("\n");
	}

	void print(set<int> s){
		set<int>::iterator it;
		for(it = s.begin(); it != s.end(); it++){
			printf("%d ", *it);
		}
		printf("\n");
	}

	void printLeaders(){
		print(leaderLines);
	}

	void printVariables(){
		for(int i=0; i<varLines.size();i++)
			cout << varLines[i] << endl;
		printf("\n");
	}

	void createBasicBlocks(){
		set<int>::iterator it, next;
		int size = leaderLines.size();
		it = leaderLines.begin();
		int cnt = 0;
		int start, end;
		while(cnt < size-1){
			start = *it;
			it++;
			next = it;
			end = *next;
			block.push_back(BasicBlock(start, end-1));
			cnt++;
		}

		block.push_back(BasicBlock(*it, nInstrunctions-1));
	}

	void printBasicBlocks(){
		string bb;
		char str[10];
		int i, j, cnt;
		cnt = 0;
		i = 0;
		j = -1;

		string space = "  ";

		while(i<block.size()-1){
			// Print BBx
			bb.append("BB");
			sprintf(str,"%d", cnt);

			bb.append(str);
			printf("%s\n", bb.c_str());
			bb.clear();
			cnt++;

			j = block[i].startLine;
			while(j < block[i+1].startLine){
				cout << space << instructions[j].ins << endl;
				j++;
			}
			i++;
		}

		// print the last basic block
			// Print BBx
		bb.append("BB");
		sprintf(str,"%d", cnt);
		bb.append(str);
		printf("%s\n", bb.c_str());

		j = block[block.size()-1].startLine;
		while(j < nInstrunctions){
			cout << space << instructions[j].ins << endl;
			j++;
		}
		nBasicBlock = cnt+1;
	}

	int getLineNoFromLabel(string label){
		if(labelToLineNoMap.find(label) != labelToLineNoMap.end())
			return labelToLineNoMap.find(label)->second;

		// If the label is not cached yet, return -1, adjust the numbers later
		return -1;
	}

	void addLabelLineMap(string label, int instructionNo){
		labelToLineNoMap[label] = instructionNo;
	}

	void addLeaderForBranchInstruction(int leaderline){
		// Target instruction of a conditional or	unconditional branch is a leader
		addLeader(leaderline);
	}

	void addLeaderForBranchInstruction(char* label){
		int leaderline = getLineNoFromLabel(label);
		if(leaderline != -1){
			addLeaderForBranchInstruction(leaderline);
		}
		// Add it to the vector for further processing at the end of parsing file
		else{
			pendingLabelLineUpdateMap.push_back(label);
			pendingLabelInstructionLineNo.push_back(nInstrunctions);
		}
	}

	void updatePendingLabelMap(){
		for(int i=0; i<pendingLabelLineUpdateMap.size(); i++){
			int leaderline = getLineNoFromLabel(pendingLabelLineUpdateMap[i]);
			if(leaderline == -1){
				cout << "Error!!" << endl;
				assert(0);
			}
			addLeader(leaderline);
		}
	}

	// Read Variable
	// Return 1 - if the line contains variable declaration
	// Return 0 - If end of variable declaration
	bool parseVariable(char* s){
		// check whether scalar or vector
		char varSign[5];
		char name[SIZE];
		int n;

		sscanf(s, "%s", varSign);
		// Vector
		if(strcmp(varSign, ".[]") == 0){
			sscanf(s, "%s%s%d", varSign, name, &n);
			// Remove the , char from the end
			name[strlen(name)-1]='\0';
			return true;
		}
		// Scalar
		else if(strcmp(varSign, ".") == 0){
			sscanf(s, "%s%s", varSign, name);
			return true;
		}
		return false;
	}

	bool parseUnconditionalBranchInstruction(char* line){
		// Unconditional Branch
		char op[5];
		char label[SIZE];
		sscanf(line, "%s%s", op, label);
		addLeaderForBranchInstruction(label);
		return true;
	}

	bool parseConditionalBranchInstruction(char* line){
		char op[5];
		char label[SIZE];
		sscanf(line, "%s%s", op, label);
		// Remove the , char from the end
		label[strlen(label)-1] = '\0';
		addLeaderForBranchInstruction(label);
		return true;
	}

	bool parseInstruction(char* line){
		char op[5];
		char label[SIZE];
		sscanf(line, "%s", op);

		// label
		if(strcmp(op, ":") == 0){
			sscanf(line, "%s%s", op, label);
			instructions[nInstrunctions].label = label;
			addLabelLineMap(string(label), nInstrunctions);
		}
		// Unconditional Branch
		else if(strcmp(op, ":=") == 0){
			instructions[nInstrunctions].instructionType = 1;
			sscanf(line, "%s%s", op, label);
			instructions[nInstrunctions].label = label;
			parseUnconditionalBranchInstruction(line);

			// Instruction immediately following a conditional or unconditional branch is also a leader
			addLeader(nInstrunctions+1);
		}
		// Conditional Branch
		else if(strcmp(op, "?:=") == 0){
			instructions[nInstrunctions].instructionType = 2;
			sscanf(line, "%s%s", op, label);
			label[strlen(label)-1] = '\0';
			instructions[nInstrunctions].label = label;
			parseConditionalBranchInstruction(line);
			// Instruction immediately following a conditional or unconditional branch is also a leader
			addLeader(nInstrunctions+1);
		}

		return true;
	}

	void parseLine(char* line){
		int lineLength;
		int instructionStartLineNo = -1;
		lineLength = strlen(line);

		if(!lineLength)
			return;

		lineNo++;
		bool varDeclare = true;
		// Read the variables
		if(varDeclare){
			nVars++;
			// Is a variable declaration statement
			if(parseVariable(line)){
				varLines.push_back(line);
				return;
			}
			// End of variable declaration statement
			else{
				varDeclare = false;
				instructionStartLineNo = lineNo;
			}
		}

		// Read Instruction
		if(!varDeclare){
			instructions.push_back(Instr(line));

			// First instruction in the program is a leader
			if(leadersIsEmpty()){
				addLeader(0);
			}

			if(!parseInstruction(line))
				cout << "Error in parsing" << endl;
			nInstrunctions++;
		}
	}

	int getBlockNoFromInstID(int instrID){
		// Cache instruction line later
		for(int i=0; i<nBasicBlock; i++){
			if(block[i].startLine <= instrID && instrID <= block[i].endLine)
				return i;
		}
		return -1;
	}

	/******************** Control Flow Graph ***********************/
	void initMemoryForCFG(int size){
		for(int i=0; i<size; i++){
			for(int j=0; j<size; j++){
				adjMat[i][j] = 0;
			}
		}
	}

	void allocateMemoryForCFG(){
		// Create 2D array
		int size = nBasicBlock+1;
		adjMat = new int*[size];
		for(int i=0; i<size; i++){
			adjMat[i] = new int[size];
		}

		initMemoryForCFG(size);
	}

	void addEdges(){
		int size = nBasicBlock;
		int cnt = 0;
		for(int i=0; i<nBasicBlock-1; i++){
			adjMat[i][i+1] = 1;
		}

		/* Add an Edge B1->B2 if B2 immediately follows B1 in program order and
			B1 does not end in an unconditional branch.
		*/
		for(int i=0; i<nBasicBlock-1; i++){
			int t = instructions[block[i].endLine].instructionType;
			// Unconditonal Branch
			if(t == 1){
				adjMat[i][i+1] = 0;
			}
		}

		/* Add an Edge B1->B2 if there is a conditional or unconditional branch from the last
			statement in B1 to the leader of B2
		*/

		string label;
		int j, instID;

		for(int i=0; i<nBasicBlock; i++){
			int t = instructions[block[i].endLine].instructionType;
			// Branch
			if(t == 1 || t == 2){
				label = instructions[block[i].endLine].label;
				// Find the instruction of the corresponding label
				instID = getLineNoFromLabel(label);

				j = getBlockNoFromInstID(instID);
				adjMat[i][j] = 1;
			}
		}
	}

	void printEdges(){
		int size = nBasicBlock+1;
		int cnt = 0;
		for(int i=0; i<size; i++){
			for(int j=0; j<size; j++){
				if(adjMat[i][j]){
					printf("EDGE %d: BB%d -> BB%d\n", cnt++, i, j);
				}
			}
		}
	}

	// Compute parents for all nodes
	void computeParents(){
		for(int i=0; i<nBasicBlock; i++){
			for(int j=0; j<nBasicBlock; j++){
				// If there is an edge between i -> j, then i is the parent of j
				if(adjMat[i][j]){
					parents[j].push_back(i);
				}
			}
		}
	}

	void printParentList(){
		for(int i=0; i<nBasicBlock; i++){
			printf("Parents of Node %d: ", i+1);
			print(parents[i], 1);
		}
	}

	void allocateParentList(){
		parents = new vector<int> [nBasicBlock+1];
		computeParents();
	}

	/******************** Dominator Sets ***********************/

	void initMemoryForDominatorSets(){
		domSet[0].push_back(0);

		// startnode n0 = 0
		vector<int> allNodes;
		for(int i=0; i<nBasicBlock; i++)
			allNodes.push_back(i);

		// For all n in N, n != n0, initialize D(n0) = N
		for(int i=1; i<nBasicBlock; i++){
			domSet[i] = allNodes;
		}
	}

	void allocateMemoryForDominatorSets(){
		// Create 2D array
		domSet = new vector<int> [nBasicBlock+1];

		initMemoryForDominatorSets();
	}

	void calculateIntersectionofPredecessorDom(int n, vector<int>& predDom){
		vector<int> predecessor = parents[n];
		if(predecessor.size() < 1){
			return;
		}

		set<int> intersectionDom(domSet[predecessor[0]].begin(), domSet[predecessor[0]].end());

		vector<int>::iterator it;
		vector<int> t;
		vector<int> result;
		result.resize(intersectionDom.size(), 0);

		for(int i=1; i<predecessor.size(); i++){
			t = domSet[predecessor[i]];
			sort(t.begin(), t.end());

			it = set_intersection(intersectionDom.begin(), intersectionDom.end(), t.begin(), t.end(), result.begin());
			result.resize(it-result.begin());

			intersectionDom.clear();
			intersectionDom.insert(result.begin(), result.end());
		}

		// copy result to predecessor dominator vector
		predDom.clear();

		predDom.resize(intersectionDom.size());
		copy(intersectionDom.begin(), intersectionDom.end(), predDom.begin());
	}

	bool noChange(vector<int> cur, vector<int> prev){
		vector<int> diff;
		vector<int>::iterator it;

		if(cur.size() != prev.size())
			return false;

		diff.clear();
		it = set_difference(cur.begin(), cur.end(), prev.begin(), prev.end(), diff.begin());
		diff.resize(it - diff.begin());

		if(diff.size() == 0)
			return true;

		return false;
	}

	void computeDominatorSets(){
		vector<int> predDom;
		vector<int> curDomSet;

		int cnt = 0;

		bool change = true;
		while(change){
			change = false;
			// For all Nodes
			for(int n=1; n<nBasicBlock; n++){
				curDomSet.clear();

				// intersection of predecessors of node n's dominator sets
				calculateIntersectionofPredecessorDom(n, predDom);

				// Append predDom to current dominator set
				for(int i=0; i<predDom.size(); i++)
					curDomSet.push_back(predDom[i]);

				// if n is not in the set, insert n
				if(find(curDomSet.begin(), curDomSet.end(), n) == curDomSet.end()){
					curDomSet.push_back(n);
				}
				sort(curDomSet.begin(), curDomSet.end());

				if(!noChange(domSet[n], curDomSet)){
						change = true;
				}
				domSet[n] = curDomSet;
			}
			cnt++;
		}
	}

	void printDominatorSets(){
		for(int i=0; i<nBasicBlock; i++){
			printf("Dominator set of Node %d: ", i);
			print(domSet[i], 0);
		}
	}

	void printBackEdges(){
		printf("Set of Back Edges\n");
		for(int i=0; i<backEdges.size(); i++){
			Edge e = backEdges[i];
			printf("%d -> %d\n", e.x, e.y);
		}
	}

	void identifyBackEdges(){
		int a, b;
		for(int n=0; n<nBasicBlock; n++){
			for(int i=0; i<domSet[n].size(); i++){
				// a dominates b
				a = n;
				b = domSet[n][i];

				// Identify Back Edge, check if there is an edge a->b, then a -> b is back edge
				if(adjMat[a][b] == 1){
					backEdges.push_back(Edge(a, b));
				}
			}
		}
	}

	/********************* Loop Construction *********************/
	void allocateMemoryForLoops(){
		// Create 2D array
		loops = new vector<int> [backEdges.size()];

		for(int i=0; i<backEdges.size(); i++)
			loops[i].clear();
	}


	void insert(int m){
		vector<int>::iterator it;

		it = find(loop.begin(), loop.end(), m);
		// Insert Node N
		if(it == loop.end()){
			loop.push_back(m);
			S.push(m);
		}
	}

	// Insert parents of node n into stack
	void insertParentintoStack(int n){
		vector<int> predecessor = parents[n];
		if(predecessor.size() < 1){
			return;
		}

		for(int i=0; i<predecessor.size(); i++){
			insert(predecessor[i]);
		}
	}
	// Given a back edge N->D, construct loop
	void constructLoop(Edge e){
		// clear stack
		while(!S.empty()){
			S.pop();
		}

		// clear temporary loop
		loop.clear();

		// Push D
		loop.push_back(e.y);

		// insert N
		insert(e.x);

		// while stack is not empty
		while(!S.empty()){
			// pop top element
			int m = S.top();
			S.pop();
			insertParentintoStack(m);
		}
	}

	void constructAllLoops(){
		for(int i=0; i<backEdges.size(); i++){
			Edge e = backEdges[i];
			constructLoop(backEdges[i]);
			loops[i] = loop;
			sort(loops[i].begin(), loops[i].end());
		}
	}

	void printLoops(){
		if(backEdges.size() > 1)
			sort(loops, loops+backEdges.size());
		for(int i=0; i<backEdges.size(); i++){
			printf("LOOP %d: ", i);
			print(loops[i], 0, "BB");
		}
	}

	void constructCFG(){
		updatePendingLabelMap();

		//printVariables();
		createBasicBlocks();
		// print 2 spaces before the line - as done in sample outputs
		printBasicBlocks();
		printf("\n");

		allocateMemoryForCFG();
		addEdges();
		printf("\n");
		printEdges();
	}

	void computeDominators(){
		allocateMemoryForDominatorSets();
		allocateParentList();

		computeDominatorSets();
		//printDominatorSets();

		identifyBackEdges();
		//printBackEdges();

		// If no back edges, no loop
		if(backEdges.size() < 1)
			return;

		printf("\n");
		allocateMemoryForLoops();
		constructAllLoops();
		printLoops();
	}

	~CFG(){

		if(adjMat){
			int size = nBasicBlock+1;
			for( int i=0 ; i<size; i++){
				delete [] adjMat[i] ;
			}
			delete[] adjMat;
		}

		// Delete dominator Set memory
		if(domSet){
			delete[] domSet;
		}

		// Delete parents set
		if(parents){
			delete[] parents;
		}
	}
};

// Detect Loops

int main(int argc, char *argv[]){
	CFG cfg;

	string fileName;
	string outputFileName;

	if(argc > 1){
		fileName = string(argv[1]);
	}
	if(argc > 2)
		outputFileName = string(argv[2]);

	char line[SIZE];
	if(!fileName.empty())
		freopen(fileName.c_str(), "r", stdin);

	if(!outputFileName.empty())
		freopen(outputFileName.c_str(), "w", stdout);

	while(gets(line)){
		cfg.parseLine(line);
	}

	cfg.constructCFG();
	cfg.computeDominators();

	fclose(stdin);
	fclose(stdout);

	return 0;
}
