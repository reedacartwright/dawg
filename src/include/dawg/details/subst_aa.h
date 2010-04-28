#pragma once
#ifndef DAWG_SUBST_AA_H
#define DAWG_SUBST_AA_H
/****************************************************************************
 *  Copyright (C) 2009-2010 Reed A. Cartwright, PhD <reed@scit.us>          *
 ****************************************************************************/
 
namespace dawg {
 
// name, followed by params, then freqs
template<typename It1, typename It2>
bool subst_model::create_aagtr(const char *mod_name, It1 first1, It1 last1, It2 first2, It2 last2) {
	double d = 0.0;
	int u = 0;
	_model = residue_exchange::AA;
	// do freqs first
	if(!create_freqs(mod_name, first2, last2, &freqs[0], &freqs[20]))
		return false;
	
	// fill params array
	double params[190];
	u = 0;
	for(;first1 != last1 && u<190;++first1,++u) {
		if(*first1 < 0)
			return DAWG_ERROR("Invalid subst model; aagtr parameter #" << u
				<< " '" << *first1 << "' is not >= 0.");
		params[u] = *first1;
	}
	if(u != 190)
		return DAWG_ERROR("Invalid subst model; aagtr requires 190 parameters.");
	
	// construct substitution matrix
	// do this locally to enable possible optimizations?
	double s[20][20];
	double rs[20];
	u = 0;
	for(int i=0;i<20;++i) {
		s[i][i] = 0.0;
		for(int j=i+1;j<20;++j) {
			s[i][j] = s[j][i] = params[u++];
		}
	}
	// scale the matrix to substitution time and uniformize
	d = 0.0;
	uni_scale = 0.0;
	for(int i=0;i<20;++i) {
		for(int j=0;j<20;++j) {
			s[i][j] *= freqs[j];
			d += s[i][j]*freqs[i];
		}
	}
	for(int i=0;i<20;++i) {
		rs[i] = 0.0;
		for(int j=0;j<20;++j) {
			s[i][j] /= d;
			rs[i] += s[i][j];
		}
		uni_scale = std::max(uni_scale, rs[i]);
	}
	// create pseudosubstitutions and transition frequencies
	for(int i=0;i<20;++i)
		s[i][i] = uni_scale - rs[i];
	for(int i=0;i<20;++i) {
		for(int j=0;j<20;++j)
			s[i][j] /= uni_scale;
	}
	
	// create cumulative frequencies
	d = 0.0;
	for(int i=0;i<19;++i) {
		d += freqs[i];
		freqs[i] = d;
	}
	// we will include 32 sites in our binary search
	// so fill them with 1.0
	std::fill(&freqs[19],&freqs[31], 1.0);
	freqs[19] = 1.0;
	for(int i=0;i<20;++i) {
		d = 0.0;
		for(int j=0;j<19;++j) {
			d += s[i][j];
			table[i][j] = d;
		}
		// we will include 32 sites in our binary search
		// so fill them with 1.0
		std::fill(&table[i][19],&table[i][31], 1.0);
	}
	name = mod_name;
	do_op_f = &subst_model::do_aagtr_f;
	do_op_s = &subst_model::do_aagtr_s;
	
	return true;
}

template<typename It1, typename It2>
bool subst_model::create_wag(const char *, It1 first1, It1 last1, It2 first2, It2 last2) {
	static const double s[190] = {
		1.02704, 0.738998, 0.0302949, 1.58285, 0.021352, 6.17416, 0.210494, 0.39802,
		0.0467304, 0.0811339, 1.41672, 0.306674, 0.865584, 0.567717, 0.049931, 0.316954,
		0.248972, 0.930676, 0.570025, 0.679371, 0.24941, 0.193335, 0.170135, 0.039437,
		0.127395, 1.05947, 0.0304501, 0.13819, 0.906265, 0.0740339, 0.479855, 2.58443,
		0.088836, 0.373558, 0.890432, 0.323832, 0.397915, 0.384287, 0.0848047, 0.154263,
		2.11517, 0.0613037, 0.499462, 3.17097, 0.257555, 0.893496, 0.390482, 0.103754,
		0.315124, 1.19063, 0.1741, 0.404141, 4.25746, 0.934276, 4.85402, 0.509848,
		0.265256, 5.42942, 0.947198, 0.0961621, 1.12556, 3.95629, 0.554236, 3.01201,
		0.131528, 0.198221, 1.43855, 0.109404, 0.423984, 0.682355, 0.161444, 0.24357,
		0.696198, 0.0999288, 0.556896, 0.415844, 0.171329, 0.195081, 0.908598, 0.0988179,
		0.616783, 5.46947, 0.0999208, 0.330052, 4.29411, 0.113917, 3.8949, 0.869489,
		1.54526, 1.54364, 0.933372, 0.551571, 0.528191, 0.147304, 0.439157, 0.102711,
		0.584665, 2.13715, 0.186979, 5.35142, 0.497671, 0.683162, 0.635346, 0.679489,
		3.0355, 3.37079, 1.40766, 1.07176, 0.704939, 0.545931, 1.34182, 0.740169, 0.31944,
		0.96713, 0.344739, 0.493905, 3.97423, 1.61328, 1.02887, 1.22419, 2.12111, 0.512984,
		0.374866, 0.822765, 0.171903, 0.225833, 0.473307, 1.45816, 1.38698, 0.326622,
		1.51612, 2.03006, 0.795384, 0.857928, 0.554413, 4.37802, 2.00601, 1.00214, 0.152335,
		0.588731, 0.649892, 0.187247, 0.118358, 7.8213, 0.305434, 1.80034, 2.05845, 0.196246,
		0.314887, 0.301281, 0.251849, 0.232739, 1.38823, 0.113133, 0.71707, 0.129767,
		0.156557, 1.52964, 0.336983, 0.262569, 0.212483, 0.137505, 0.665309, 0.515706,
		0.0719167, 0.139405, 0.215737, 1.16392, 0.523742, 0.110864, 0.365369, 0.240735,
		0.543833, 0.325711, 0.196303, 6.45428, 0.103604, 3.87344, 0.42017, 0.133264, 0.398618,
		0.428437, 1.086, 0.216046, 0.22771, 0.381533, 0.786993, 0.291148, 0.31473, 2.48539
	};
	static const double p[20] = {
		0.0866279, 0.0193078, 0.0570451, 0.0580589, 0.0384319, 0.0832518, 0.0244313, 0.048466,
		0.0620286, 0.086209, 0.0195027, 0.0390894, 0.0457631, 0.0367281, 0.043972, 0.0695179,
		0.0610127, 0.0708956, 0.0143859, 0.0352742
	};
	if(first2 != last2) { //+F model
		return create_aagtr("wag+f", &s[0], &s[190], first2, last2);
	}
	return create_aagtr("wag", &s[0], &s[190], &p[0], &p[20]);
}

template<typename It1, typename It2>
bool subst_model::create_wagstar(const char *, It1 first1, It1 last1, It2 first2, It2 last2) {
	static const double s[190] = {
		1.21324, 0.731152, 0.0379056, 1.55788, 0.0284956, 6.04299, 0.213179, 0.485001,
		0.0458258, 0.0873936, 1.41993, 0.312544, 0.88357, 0.588609, 0.0552962, 0.317684,
		0.341479, 0.958529, 0.599188, 0.631713, 0.279542, 0.214596, 0.198958, 0.0390513,
		0.124553, 1.06458, 0.0310522, 0.162975, 0.881639, 0.0719929, 0.480308, 2.45392,
		0.0832422, 0.381514, 0.854485, 0.320597, 0.400822, 0.451124, 0.0869637, 0.154936,
		2.10414, 0.067443, 0.508952, 3.1554, 0.255092, 0.887458, 0.428648, 0.0992829,
		0.294481, 1.14516, 0.184545, 0.40117, 3.94646, 0.877057, 4.81956, 0.514347, 0.233527,
		5.30821, 1.00122, 0.0848492, 1.12717, 3.9337, 0.527321, 2.88102, 0.144354, 0.198404,
		1.51861, 0.109081, 0.444152, 0.720567, 0.165205, 0.254626, 0.722123, 0.111722, 0.588203,
		0.422851, 0.179858, 0.204905, 1.03344, 0.0999068, 0.657364, 5.6037, 0.109241, 0.346823,
		4.87366, 0.125999, 4.19125, 0.873266, 1.64018, 1.62299, 0.913179, 0.589718, 0.568449,
		0.159054, 0.443685, 0.122792, 0.629768, 2.31211, 0.187262, 5.74119, 0.51821, 0.660816,
		0.67416, 0.711498, 3.02808, 3.52499, 1.35221, 1.09965, 0.822025, 0.563999, 1.33618,
		0.876688, 0.321774, 1.05314, 0.351913, 0.554077, 3.90127, 1.54694, 0.87908, 1.35611,
		2.24161, 0.522957, 0.395176, 0.889765, 0.188237, 0.236489, 0.54992, 1.48876, 1.45173,
		0.351564, 1.56873, 2.06787, 0.802531, 0.829315, 0.594177, 4.02507, 1.92496, 1.10899,
		0.155419, 0.588443, 0.653015, 0.190095, 0.119749, 7.48376, 0.300343, 1.82105, 2.03324,
		0.193323, 0.325745, 0.32893, 0.282892, 0.23769, 1.4088, 0.135395, 0.728065, 0.142159,
		0.176397, 1.58681, 0.366467, 0.261223, 0.259584, 0.159261, 0.706082, 0.565299, 0.0746093,
		0.135024, 0.208163, 1.24086, 0.528249, 0.118584, 0.396884, 0.270321, 0.481954, 0.326191,
		0.209621, 6.49269, 0.108982, 4.31772, 0.44009, 0.155623, 0.427718, 0.437069, 1.05269,
		0.212945, 0.210494, 0.386714, 0.742154, 0.286443, 0.353358, 2.42261
	};
	static const double p[20] = {
		0.0866279, 0.0193078, 0.0570451, 0.0580589, 0.0384319, 0.0832518, 0.0244313, 0.048466,
		0.0620286, 0.086209, 0.0195027, 0.0390894, 0.0457631, 0.0367281, 0.043972, 0.0695179,
		0.0610127, 0.0708956, 0.0143859, 0.0352742
	};
	if(first2 != last2) { //+F model
		return create_aagtr("wagstar+f", &s[0], &s[190], first2, last2);
	}
	return create_aagtr("wagstar", &s[0], &s[190], &p[0], &p[20]);
}

template<typename It1, typename It2>
bool subst_model::create_lg(const char *, It1 first1, It1 last1, It2 first2, It2 last2) {
	static double s[190];
	static double p[20];
	if(first2 != last2) { //+F model
		return create_aagtr("lg+f", &s[0], &s[190], first2, last2);
	}
	return create_aagtr("lg", &s[0], &s[190], &p[0], &p[20]);
}

template<typename It1, typename It2>
bool subst_model::create_jtt(const char *, It1 first1, It1 last1, It2 first2, It2 last2) {
	static double s[190];
	static double p[20];
	if(first2 != last2) { //+F model
		return create_aagtr("jtt+f", &s[0], &s[190], first2, last2);
	}
	return create_aagtr("jtt", &s[0], &s[190], &p[0], &p[20]);
}

template<typename It1, typename It2>
bool subst_model::create_dayhoff(const char *, It1 first1, It1 last1, It2 first2, It2 last2) {
	static double s[190];
	static double p[20];
	if(first2 != last2) { //+F model
		return create_aagtr("dayhoff+f", &s[0], &s[190], first2, last2);
	}
	return create_aagtr("dayhoff", &s[0], &s[190], &p[0], &p[20]);
}

template<typename It1, typename It2>
bool subst_model::create_molphy(const char *, It1 first1, It1 last1, It2 first2, It2 last2) {
	static double s[190];
	static double p[20];
	if(first2 != last2) { //+F model
		return create_aagtr("molphy+f", &s[0], &s[190], first2, last2);
	}
	return create_aagtr("molphy", &s[0], &s[190], &p[0], &p[20]);
}

} // namespace dawg
 
#endif // DAWG_SUBST_AA_H
 
