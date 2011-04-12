#ifndef CONFIG_BASE_H__
#define CONFIG_BASE_H__

#include "definitions.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <tr1/unordered_map>

// TODO: remove
using namespace std;
using namespace std::tr1;

namespace pgibbs {

#define DIE_HELP(msg) do {                      \
    std::ostringstream oss;                     \
    oss << msg;                                 \
    dieOnHelp(oss.str()); }                     \
  while (0);

class ConfigBase {

protected:

    // name -> value, description
    typedef unordered_map<string, pair<string,string> > ConfigMap;

    // argument functions
    int minArgs_, maxArgs_;   // min and max number of main arguments
    vector<string> mainArgs_; // the main non-optional argiments
    ConfigMap optArgs_;       // optional arguments 

    // details for printing the usage
    string usage_;            // usage details
    vector<string> argOrder_;

public:

    ConfigBase() : minArgs_(0), maxArgs_(255) { 
		addConfigEntry("iters",   "10", "The number of iterations to perform");
		addConfigEntry("threads", "1",  "The number of threads to use");
		addConfigEntry("blocksize",  "1",  "The size of one block (for blocked sampling)");
		addConfigEntry("shuffle",  "false",  "Whether to shuffle");
		addConfigEntry("skipmh",  "false",  "Whether to skip the Metropolis-Hastings step");
		addConfigEntry("printmod",  "false",  "Whether to print the model probabilities");
		addConfigEntry("sampmeth", "sequence",  "Sampling method (sequence,parallel,block)");
    }

    void dieOnHelp(const string & str) const {
        // print arguments
        cerr << usage_ << endl;
        cerr << "Arguments: "<<endl;
        for(vector<string>::const_iterator it = argOrder_.begin(); it != argOrder_.end(); it++) {
            ConfigMap::const_iterator oit = optArgs_.find(*it);
            if(oit->second.second.length() != 0)
                cerr << " -"<<oit->first<<" \t"<<oit->second.second<<endl;
        }
        cerr << endl << str << endl;
        exit(1);
    }
    
    void printConf() const {
        // print arguments
        cout << "Main arguments:" << endl;
        for(int i = 0; i < (int)mainArgs_.size(); i++)
            cout << " "<<i<<": "<<mainArgs_[i]<<endl;
        cout << "Optional arguments:"<<endl;
        for(vector<string>::const_iterator it = argOrder_.begin(); it != argOrder_.end(); it++) {
            ConfigMap::const_iterator oit = optArgs_.find(*it);
            if(oit->second.second.length() != 0)
                cerr << " -"<<oit->first<<" \t"<<oit->second.first<<endl;
        }
    }

    vector<string> loadConfig(int argc, char** argv) {
        for(int i = 1; i < argc; i++) {
            if(argv[i][0] == '-') {
                string name(argv[i]+1); 
                ConfigMap::iterator cit = optArgs_.find(name);
                if(cit == optArgs_.end())
                    DIE_HELP("Illegal argument "<<name);
                if(i == argc-1 || argv[i+1][0] == '-')
                    cit->second.first = "true";
                else
                    cit->second.first = argv[++i];
            }
            else
                mainArgs_.push_back(argv[i]);
        }

        // sanity checks
        if((int)mainArgs_.size() < minArgs_ || (int)mainArgs_.size() > maxArgs_) {
            DIE_HELP("Wrong number of arguments");
        }
        

        printConf();
        return mainArgs_;
    }

    void addConfigEntry(const string & name, const string & val, const string & desc) {
        argOrder_.push_back(name);
        pair<string,pair<string,string> > entry(name,pair<string,string>(val,desc));
        optArgs_.insert(entry);
    }

    // getter functions
    string getString(const string & name) const {
        ConfigMap::const_iterator it = optArgs_.find(name);
        if(it == optArgs_.end())
            THROW_ERROR("Requesting bad argument "<<name<<" from configuration");
        return it->second.first;
    }
    int getInt(const string & name) const {
        string str = getString(name);
        int ret = atoi(str.c_str());
        if(ret == 0 && str != "0")
            DIE_HELP("Value '"<<str<<"' for argument "<<name<<" was not an integer");
        return ret;
    }
    double getDouble(const string & name) const {
        string str = getString(name);
        double ret = atof(str.c_str());
        if(ret == 0 && str != "0" && str != "0.0")
            DIE_HELP("Value '"<<str<<"' for argument "<<name<<" was not float");
        return ret;
    }
    bool getBool(const string & name) const {
        string str = getString(name);
        if(str == "true") return true;
        else if(str == "false") return false;
        DIE_HELP("Value '"<<str<<"' for argument "<<name<<" was not boolean");
        return false;
    }

    // setter functions
    void setString(const string & name, const string & val) {
        ConfigMap::iterator it = optArgs_.find(name);
        if(it == optArgs_.end())
            THROW_ERROR("Setting bad argument "<<name<<" in configuration");
        it->second.first = val;
    }
    void setInt(const string & name, int val) {
        ostringstream oss; oss << val; setString(name,oss.str());
    }
    void setDouble(const string & name, double val) {
        ostringstream oss; oss << val; setString(name,oss.str());
    }
    void setBool(const string & name, bool val) {
        ostringstream oss; oss << val; setString(name,oss.str());
    }

    void setUsage(const string & str) { usage_ = str; }

    vector<string> getMainArgs() { return mainArgs_; }
	
};

}

#endif
