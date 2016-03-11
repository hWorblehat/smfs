/*
 * Rule.h
 *
 *  Created on: 5 Mar 2016
 *      Author: rowan
 */

#ifndef RULE_H_
#define RULE_H_

#include <memory>
#include <string>

using namespace std;

namespace smfs {

class Rule {

public:
	static Rule* create();

	virtual ~Rule() = 0;

	/**
	 * Transforms a path according to the rule.
	 * @param relative The path to transform.
	 * @return The transformed path.
	 */
	virtual string transform(string const &relative) const = 0;

};

} /* namespace smfs */

#endif /* RULE_H_ */
