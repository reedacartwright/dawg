#include "dawg.h"
#include "tree.h"
#include "rand.h"

using namespace std;

////////////////////////////////////////////////////////////
//  class NewickNode
////////////////////////////////////////////////////////////
NewickNode::NewickNode(NewickNode* p, const char *cs, double d) : m_dLen(d), m_pSub(p)
{
	if(cs)
		m_ssLabel = cs;
	else
		MakeName();
}

void NewickNode::MakeName()
{
	vector<const std::string*> v;
	NewickNode* p = m_pSub.get();
	while(p != NULL)
	{
		v.push_back(&p->m_ssLabel);
		p = p->m_pSib.get();
	}
	std::sort(v.begin(), v.end());
	m_ssLabel = "(";
	m_ssLabel += *v.front();
	for(vector<const std::string*>::iterator it = v.begin()+1; it != v.end(); ++it)
	{
		m_ssLabel += ",";
		m_ssLabel += **it;	
	}
	m_ssLabel += ")";
}

////////////////////////////////////////////////////////////
//  class Sequence
////////////////////////////////////////////////////////////
Sequence::Sequence()
{

}

Sequence::Sequence(const DNAVec &dna) : m_vDNA(dna), m_vHistory(dna.size(), '.')
{

}

unsigned long Sequence::GapPos(unsigned long uPos) const
{
	vector<char>::const_iterator it=m_vHistory.begin();
	unsigned long v=0;
	//skip leading 'gapspace'
	for(;(*it == '-' || *it == '=') && it != m_vHistory.end(); ++it)
		v++;
	for(; uPos && it != m_vHistory.end(); ++it)
	{
		v++;
		if(*it != '-' && *it != '=')
			uPos--;
	}
	return v;
}

unsigned long Sequence::Insert(unsigned long uPos, DNAVec::const_iterator itBegin, DNAVec::const_iterator itEnd)
{
	unsigned long uSize = (unsigned long)(itEnd-itBegin);
	if(uSize == 0 || uPos > m_vDNA.size())
		return 0;
	m_vHistory.insert(m_vHistory.begin()+GapPos(uPos), uSize, '+');
	m_vDNA.insert(m_vDNA.begin()+uPos, itBegin, itEnd);
	return uSize;
}

unsigned long Sequence::Delete(unsigned long uPos, unsigned long uSize)
{
	if(uSize == 0)
		return 0;
	uPos++;
	unsigned long uStart = (uSize < uPos)? uPos-uSize : 0u;
	unsigned long uEnd = (m_vDNA.size() > uPos) ? uPos : m_vDNA.size();
	
	m_vDNA.erase(m_vDNA.begin()+uStart, m_vDNA.begin()+uEnd);

	unsigned long uTemp = uSize = uEnd-uStart;
	uPos = GapPos(uStart);
	while(uTemp)
	{
		switch(m_vHistory[uPos])
		{
		case '=':
		case '-':
			break;
		case '+':
			m_vHistory[uPos] = '=';
			uTemp--;
			break;
		default:
			m_vHistory[uPos] = '-';
			uTemp--;
			break;
		};
		uPos++;
	}
	return uSize;
}

void Sequence::Append(const Sequence &seq)
{
		m_vDNA.insert(m_vDNA.end(), seq.m_vDNA.begin(), seq.m_vDNA.end());
		m_vHistory.insert(m_vHistory.end(), seq.m_vHistory.begin(), seq.m_vHistory.end());
}

void Sequence::ResetHistory()
{
	m_vHistory.assign(m_vHistory.size(), '.');
}

////////////////////////////////////////////////////////////
//  class Tree::Node
////////////////////////////////////////////////////////////

unsigned long Tree::Node::SeqLength() const
{
	unsigned long uRet = 0;
	for(vector<Sequence>::const_iterator it = m_vSections.begin(); it != m_vSections.end(); ++it)
		uRet += it->Length();
	return uRet;
}

unsigned long Tree::Node::Insert(unsigned long uPos,
			Sequence::DNAVec::const_iterator itBegin, Sequence::DNAVec::const_iterator itEnd)
{
	unsigned long uSize = (unsigned long)(itEnd-itBegin);

	if(uSize == 0)
		return 0;	
	vector<Sequence>::iterator it;
	for(it = m_vSections.begin(); it != m_vSections.end() && uPos > it->Length(); ++it)
			uPos -= it->Length();
	if( it == m_vSections.end())
		return 0;
	else if(uPos < it->Length())
		return it->Insert(uPos, itBegin, itEnd);
	else
	{
		// Insertion occurs at a gap between sections
		// Randomly allocate uSize among the sections
		vector<unsigned long> vTemp;
		vTemp.push_back(0);
		vTemp.push_back(rand_ulong(uSize));
		for(vector<Sequence>::iterator jt = it; jt != m_vSections.end() && jt->Length(); ++jt)
			vTemp.push_back(rand_ulong(uSize));
		vTemp.push_back(uSize);
		sort(vTemp.begin(), vTemp.end());
		unsigned long uTemp = it->Insert(uPos, itBegin+vTemp[0], itBegin+vTemp[1]);
		int i=1;
		for(vector<Sequence>::iterator jt = it+1; jt != m_vSections.end() && jt->Length(); ++jt, ++i)
			uTemp += jt->Insert(0, itBegin+vTemp[i], itBegin+vTemp[i+1]);
		return uTemp;
	}
}

unsigned long Tree::Node::Delete(unsigned long uPos, unsigned long uSize)
{
	if(uSize == 0)
		return 0;
	vector<Sequence>::iterator it;
	for(it = m_vSections.begin(); it != m_vSections.end() && uPos > it->Length(); ++it)
			uPos -= it->Length();
	if( it == m_vSections.end())
		return 0;
	return it->Delete(uPos, uSize);
}

void Tree::Node::Flatten(Sequence& seq) const
{
	for(vector<Sequence>::const_iterator cit = m_vSections.begin();
		cit != m_vSections.end(); ++cit)
		seq.Append(*cit);
}


////////////////////////////////////////////////////////////
//  class Tree
////////////////////////////////////////////////////////////

void Tree::ProcessTree(NewickNode* pNode)
{
	m_map["_R()()T"];
	ProcessNewickNode(pNode, m_map.find("_R()()T"));
	m_nSec++;
}

void Tree::ProcessNewickNode(NewickNode* pNode, Node::Handle hAnc)
{
	if(pNode->m_pSib.get())
		ProcessNewickNode(pNode->m_pSib.get(), hAnc);

	Node& node = m_map[pNode->m_ssLabel];
	node.m_mBranchLens[hAnc] = pNode->m_dLen;
	node.m_vAncestors.resize(m_nSec+1, m_map.end());
	node.m_vAncestors[m_nSec] = hAnc;
			
	if(pNode->m_pSub.get())
		ProcessNewickNode(pNode->m_pSub.get(), m_map.find(pNode->m_ssLabel));
}

void Tree::Evolve()
{
	// Reset Sequences
	for(Node::Map::iterator it=m_map.begin(); it!=m_map.end();++it)
	{
		it->second.m_vSections.clear();
		it->second.m_bTouched = false;
	}
	// Setup Root
	Node& rNode = m_map["_R()()T"];
	rNode.m_bTouched = true;
	for(vector<Sequence::DNAVec>::iterator it = m_vDNASeqs.begin();
		it != m_vDNASeqs.end(); ++it)
	{
		rNode.m_vSections.push_back(Sequence(*it));
		for(unsigned int u = 0;u<it->size();++u)
		{
			if(rNode.m_vSections.back()[u].m_dRate < 0.0)
				rNode.m_vSections.back()[u].m_dRate = RandomRate();
			if(rNode.m_vSections.back()[u].m_nuc >= 4)
				rNode.m_vSections.back()[u].m_nuc = RandomNuc();
		}
	}

	for(Node::Handle it = m_map.begin(); it != m_map.end(); ++it)
		Evolve(it->second);
}

void Tree::Evolve(Node &rNode)
{
	if(rNode.m_bTouched)
		return;
	rNode.m_bTouched = true;
	map<Node::Handle, Node> mapSeqs;
	for(unsigned long a = 0; a < rNode.m_vAncestors.size(); ++a)
	{
		if(mapSeqs.find(rNode.m_vAncestors[a]) == mapSeqs.end())
		{
			// Touch Ancestor, make sure it exists
			Evolve(rNode.m_vAncestors[a]->second);
			// Copy ancestor to temporary location
			mapSeqs[rNode.m_vAncestors[a]] = rNode.m_vAncestors[a]->second;
			// Evolve temporary location
			Evolve(mapSeqs[rNode.m_vAncestors[a]], rNode.m_mBranchLens[rNode.m_vAncestors[a]]);
		}
		// Assemble final sequence
		rNode.m_vSections.push_back(mapSeqs[rNode.m_vAncestors[a]].m_vSections[a]);
	}
}

void Tree::Evolve(Node &rNode, double dTime)
{
	dTime = fabs(dTime);
	if(dTime < DBL_EPSILON)
		return; // Nothing to evolve
	
	// Substitutions
	for(vector<Sequence>::iterator it = rNode.m_vSections.begin(); it != rNode.m_vSections.end(); ++it)
		for(unsigned long u = 0; u < it->Length(); ++u)
		{
			if((*it)[u].m_dRate < DBL_EPSILON)
				continue; // Invariant Site
			double dTemp = dTime*(*it)[u].m_dRate;
			if(dTemp != m_dOldTime)
			{
				m_dOldTime = dTemp;
				Vector4  vec;
				vec[0] = exp(dTemp*m_vecL[0]);
				vec[1] = exp(dTemp*m_vecL[1]);
				vec[2] = exp(dTemp*m_vecL[2]);
				vec[3] = exp(dTemp*m_vecL[3]);
				Matrix44 mat; mat.Scale(vec, m_matU);
				m_matSubst.Multiply(m_matV, mat);
				for(Matrix44::Pos i=0;i<4;++i)
				{
					m_matSubst(i,1) += m_matSubst(i,0);
					m_matSubst(i,2) += m_matSubst(i,1);
					//m_matSubst(i,3) = 1.0;
				}
			}
			dTemp = rand_real();
			if(dTemp <= m_matSubst((*it)[u].m_nuc, 0))
				(*it)[u].m_nuc = 0;
			else if(dTemp <= m_matSubst((*it)[u].m_nuc, 1))
				(*it)[u].m_nuc = 1;
			else if(dTemp <= m_matSubst((*it)[u].m_nuc, 2))
				(*it)[u].m_nuc = 2;
			else
				(*it)[u].m_nuc = 3;
		}
	// Indels
	if(m_funcRateSum(0.0) < DBL_EPSILON)
		return;

	unsigned long uLength = rNode.SeqLength();
	double dLength = (double)uLength;
	double dW = 1.0/m_funcRateSum(dLength);
	double dt = rand_exp(dW);
	while(dt <= dTime)
	{
		if(rand_bool(m_funcRateIns(dLength)*dW))
		{
			//Insertion
			Sequence::DNAVec dna(m_pInsertionModel->RandSize());
			for(Sequence::DNAVec::iterator it = dna.begin(); it != dna.end(); ++it)
				*it = RandomNucleotide();
			uLength += rNode.Insert(rand_ulong(uLength), dna.begin(), dna.end() );
		}
		else
		{
			//Deletion
			unsigned long ul = m_pDeletionModel->RandSize();
			uLength -= rNode.Delete(rand_ulong(uLength+ul-1), ul);
		}
		dLength = (double)uLength;
		dW = 1.0/m_funcRateSum(dLength);
		dt += rand_exp(dW);
	}
}

bool Tree::SetupEvolution(double pFreqs[], double pSubs[],
	const IndelModel::Params& rIns, const IndelModel::Params& rDel,
	double dGamma, double dIota, double dScale)
{
	// Verifiy Parameters
	if(pFreqs[0] < 0.0 || pFreqs[1] < 0.0 || pFreqs[2] < 0.0 || pFreqs[3] < 0.0)
		return DawgError("Nucleotide frequences need to be positive.");
	pFreqs[3] = 1.0-pFreqs[0]-pFreqs[1]-pFreqs[2];
	if( pFreqs[3] < 0.0 )
		return DawgError("Nucleotide frequencies need to sum to 1.0.");
	if(pSubs[0] < 0.0 || pSubs[1] < 0.0 || pSubs[2] < 0.0
		|| pSubs[3] < 0.0 || pSubs[4] < 0.0 || pSubs[5] < 0.0)
		return DawgError("Substitution rates need to be positive.");
	if(rIns.dLambda < 0.0)
		return DawgError("Lambda (Ins) must not be negative.");
	if(rDel.dLambda < 0.0)
		return DawgError("Lambda (Del) must not be negative.");
	if(dGamma < 0.0)
		return DawgError("Invalid Gamma, \"%f\".  Gamma must be positive.", dGamma);
	else if(0.0 > dIota || dIota > 1.0)
		return DawgError("Invalid Iota, \"%f\".  Iota must be a probability.", dIota);
	if(dScale <= 0.0)
		return DawgError("Scale must be positive.");
	
	// Setup Rate Parameters
	m_dGamma = dGamma;
	m_dIota = dIota;

	// Setup Scale
	m_dScale = dScale;

	// Setup Cumulative Frequencies
	m_dNucCumFreqs[0] = pFreqs[0];
	m_dNucCumFreqs[1] = m_dNucCumFreqs[0]+pFreqs[1];
	m_dNucCumFreqs[2] = m_dNucCumFreqs[0]+pFreqs[2];
	m_dNucCumFreqs[3] = 1.0;

	// Setup Symetric Matrix
	Matrix44 matQ(Matrix44::s_Zero);
	matQ(0,1) = matQ(1,0) = pSubs[0]; //A-C
	matQ(0,2) = matQ(2,0) = pSubs[1]; //A-G
	matQ(0,3) = matQ(3,0) = pSubs[2]; //A-T
	matQ(1,2) = matQ(2,1) = pSubs[3]; //C-G
	matQ(1,3) = matQ(3,1) = pSubs[4]; //C-T
	matQ(2,3) = matQ(3,2) = pSubs[5]; //G-T
	
	// Store Rate Matrix
	m_matR = matQ;
	
	// Create GTR Genetating Matrix
	Vector4 vecF(pFreqs);	
	matQ.Scale(matQ, vecF);

	// Scale such that the total rate of substitution is equal to one
	matQ(0,0) = -(matQ(0,1)+matQ(0,2)+matQ(0,3));
	matQ(1,1) = -(matQ(1,0)+matQ(1,2)+matQ(1,3));
	matQ(2,2) = -(matQ(2,0)+matQ(2,1)+matQ(2,3));
	matQ(3,3) = -(matQ(3,0)+matQ(3,1)+matQ(3,2));
	matQ.Scale(matQ, -1.0/((1.0-m_dIota)*(vecF[0]*matQ(0,0)+vecF[1]*matQ(1,1)+
		vecF[2]*matQ(2,2)+vecF[3]*matQ(3,3))));
	
	// Store Scaled Q Matrix
	m_matQ = matQ;

	// Make Q a symetric matrix again
	Vector4 vecD, vecE;  //D*E=I
	for(Matrix44::Pos i=0;i<4;++i)
	{
		vecD[i] = sqrt(vecF[i]);
		vecE[i] = 1.0/vecD[i];
	}
	matQ.Scale(matQ, vecE);
	matQ.Scale(vecD, matQ);

	//Find EigenSystem using Jacobian Transformations
	int nRet = EigenSystem(matQ, m_vecL, m_matV);
	if(nRet == -1)
		return DawgError("Eigensystem failed to converge.");
	m_matU = m_matV;
	m_matV.Scale(vecE, m_matV);
	m_matU.Transpose();
	m_matU.Scale(m_matU, vecD);

	m_dLambdaIns = rIns.dLambda;
	if(m_dLambdaIns < DBL_EPSILON)
		m_pInsertionModel.release();
	else if(rIns.ssModel == "NB")
	{
		try {m_pInsertionModel.reset(new NegBnModel(rIns.vdModel));}
			catch(...) {return DawgError("Insertion model parameters not specified correctly.");}
	}
	else
	{
		try {m_pInsertionModel.reset(new UserModel(rIns.vdModel));}
			catch(...) {return DawgError("Insertion model parameters not specified correctly.");}
	}

	m_dLambdaDel = rDel.dLambda;
	if(m_dLambdaDel < DBL_EPSILON)
		m_pDeletionModel.release();
	else if(rDel.ssModel == "NB")
	{
		try {m_pDeletionModel.reset(new NegBnModel(rDel.vdModel));}
			catch(...) {return DawgError("Deletion model parameters not specified correctly.");}
	}
	else
	{
		try {m_pDeletionModel.reset(new UserModel(rDel.vdModel));}
			catch(...) {return DawgError("Deletion model parameters not specified correctly.");}
	}    
	m_funcRateIns.m = m_dLambdaIns;
	m_funcRateIns.b = m_dLambdaIns;
	m_funcRateSum.m = m_dLambdaDel+m_dLambdaIns;
	m_funcRateSum.b = m_dLambdaIns+m_dLambdaDel*( (m_pDeletionModel.get()) ? m_pDeletionModel->MeanSize()-1.0 : 0.0);
	return true;
}

bool Tree::SetupRoot(const std::vector<std::string> &vSeqs, const std::vector<int> &vLens,
					   const std::vector<std::vector<double> > &vRates)
{
	m_vDNASeqs.clear();
	if(vSeqs.size())
	{	
		for(vector<string>::const_iterator cit = vSeqs.begin(); cit != vSeqs.end(); ++cit)
		{
			Sequence::DNAVec dna(cit->size(), Nucleotide(5, -1.0));
			for(unsigned int u=0; u< cit->size(); ++u)
				dna[u].m_nuc = CharToNuc(cit->at(u));
			m_vDNASeqs.push_back(dna);
		}
	}
	else
	{
		for(vector<int>::const_iterator cit = vLens.begin(); cit != vLens.end(); ++cit)
			m_vDNASeqs.push_back(Sequence::DNAVec(*cit, Nucleotide(5, -1.0)));
	}
	if(vRates.size())
	{
		for(unsigned int u=0; u < m_vDNASeqs.size(); ++u)
			for(unsigned int v=0; v < m_vDNASeqs[u].size(); ++v)
				m_vDNASeqs[u][v].m_dRate = vRates[u][v];
	}
	return true;
}

Nucleotide::Nuc Tree::RandomNuc() const
{
	double d = rand_real();
	if(d <= m_dNucCumFreqs[0])
		return 0; // A
	else if(d <= m_dNucCumFreqs[1])
		return 1; // C
	else if(d <= m_dNucCumFreqs[2])
		return 2; // G
	else
		return 3; // T
}

double Tree::RandomRate() const
{
	if(m_dIota > DBL_EPSILON && rand_bool(m_dIota))
		return 0.0;  // Site Invariant
	else if(m_dGamma > DBL_EPSILON)
		return rand_gamma1(m_dGamma); // Gamma with mean 1.0 and var of m_dGamma
	else
		return 1.0;
}

// Insertion: +
// Deletion:  -
// Deleted Insertion: =
// Root Nuc: .

void Tree::Align(Alignment &aln) const
{
	// construct a table of flattened sequences
	vector<Sequence::HistoryVec> vHisTable;
	vector<Sequence::DNAVec> vDnaTable;

	for(Node::Map::const_iterator cit = m_map.begin();
		cit != m_map.end(); ++cit)
	{
		Sequence s;
		cit->second.Flatten(s);
		vHisTable.push_back(s.History());
		vDnaTable.push_back(s.DNA());
	}

	// Alignment rules:
	// Insertion & Deleted Insertion  : w/ ins, deleted ins, or gap
	// Deletion & Original Nucleotide : w/ del, original nucl

	bool bGo = true;
	for(unsigned int uCol = 0; bGo; uCol++)
	{
		bGo = false;

		// Test to see if gaps need to be inserted at this column
		for(vector<Sequence::HistoryVec>::const_iterator cit = vHisTable.begin();
			cit != vHisTable.end(); ++cit)
		{
			if(uCol >= cit->size())
				continue;
			bGo = true;
			if(cit->at(uCol) == '+' || cit->at(uCol) == '=')
			{
				for(vector<Sequence::HistoryVec>::iterator it = vHisTable.begin();
					it != vHisTable.end(); ++it)
				{
					if(uCol >= it->size())
						it->resize(uCol+1, '-');
					else if( it->at(uCol) != '+' && it->at(uCol) != '=')
						it->insert(it->begin()+uCol, 1, '-');
				}
				cit = vHisTable.end()-1;
			}
		}
	}
	unsigned int u = 0;
	for(Node::Map::const_iterator cit = m_map.begin();
		cit != m_map.end(); ++cit, ++u)
	{
		Sequence::HistoryVec& his = vHisTable[u];
		Sequence::DNAVec &dna = vDnaTable[u];
		string& ss = aln[cit->first];
		ss.resize(his.size(), '-');
		for(unsigned int uh = 0, ud=0; uh < his.size(); ++uh)
		{
			if(his[uh] == '.' || his[uh] == '+')
				ss[uh] = NucToChar(dna[ud++].m_nuc);
		}
	}

}

