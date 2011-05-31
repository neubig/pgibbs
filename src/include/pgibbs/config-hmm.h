#ifndef CONFIG_HMM_H__
#define CONFIG_HMM_H__

#include "pgibbs/config-base.h"

namespace pgibbs {

class HMMConfig : public ConfigBase {

public:

	HMMConfig() : ConfigBase() {
        minArgs_ = 2;
        maxArgs_ = 2;

        setUsage("~~~ pgibbs-hmm ~~~\n  by Graham Neubig\n\nLearns a hidden Markov model using Gibbs sampling.\n\nUsage: pgibbs-hmm INPUT_FILE OUTPUT_PREFIX\n");
		
		addConfigEntry("classes", "10", "The number of classes to use");
		addConfigEntry("usepy",   "false", "Whether to use a PY distribution (otherwise Dirichlet)");
		addConfigEntry("tstr",  "0.1", "The strength of the transition distribution.");
		addConfigEntry("estr",  "0.1", "The strength of the emission distribution.");
		addConfigEntry("tdisc", "0.0", "The discount of the transition distribution (PY only).");
		addConfigEntry("edisc", "0.0", "The discount of the emission distribution (PY only).");

		addConfigEntry("tstra",  "1.0", "The alpha for the strength of the transition distribution.");
		addConfigEntry("estra",  "1.0", "The alpha for the strength of the emission distribution.");
		addConfigEntry("tdisca", "1.0", "The alpha for the discount of the transition distribution (PY only).");
		addConfigEntry("edisca", "1.0", "The alpha for the discount of the emission distribution (PY only).");
		addConfigEntry("tstrb",  "1.0", "The beta for the strength of the transition distribution.");
		addConfigEntry("estrb",  "1.0", "The beta for the strength of the emission distribution.");
		addConfigEntry("tdiscb", "1.0", "The beta for the discount of the transition distribution (PY only).");
		addConfigEntry("ediscb", "1.0", "The beta for the discount of the emission distribution (PY only).");

        addConfigEntry("base", "uniform", "The distribution for the base measure (uniform/unigram).");
		
	}
		
};

}

#endif
