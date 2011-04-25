#ifndef CONFIG_WS_H__
#define CONFIG_WS_H__

#include "config-base.h"

namespace pgibbs {

class WSConfig : public ConfigBase {

public:

	WSConfig() : ConfigBase() {
        minArgs_ = 2;
        maxArgs_ = 2;

        setUsage("~~~ pgibbs-ws ~~~\n  by Graham Neubig\n\nPerforms unsupervised word segmentation using Gibbs sampling.\n\nUsage: pgibbs-ws INPUT_FILE OUTPUT_PREFIX\n");

        addConfigEntry("n", "2", "The n-gram size of the language model.");
        addConfigEntry("maxlen", "8", "The maximum length of a string.");
        addConfigEntry("avglen", "2.0", "The average length of a string.");
		addConfigEntry("samphyp", "false", "Whether to sample the hyperparameters.");
		addConfigEntry("usepy",   "false", "Whether to use a PY distribution (otherwise Dirichlet)");
		
		addConfigEntry("str",  "0.1", "The strength of the distribution.");
		addConfigEntry("disc", "0.0", "The discount of the distribution (PY only).");

		addConfigEntry("stra",  "1.0", "The alpha for the strength of the distribution.");
		addConfigEntry("disca", "1.0", "The alpha for the discount of the distribution (PY only).");
		addConfigEntry("strb",  "1.0", "The beta for the strength of the distribution.");
		addConfigEntry("discb", "1.0", "The beta for the discount of the distribution (PY only).");
		
	}
		
};

}

#endif
