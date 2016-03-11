/*
 * IndexedPrefixRule.h
 *
 *  Created on: 8 Mar 2016
 *      Author: Rowan Lonsdale
 */

#ifndef INDEXEDPREFIXRULE_H_
#define INDEXEDPREFIXRULE_H_

#include "Rule.h"

namespace smfs {

class IndexedPrefixRule : Rule {
public:
	static IndexedPrefixRule* create();

	virtual ~IndexedPrefixRule() override;

	string transform(string const &relative) const override;
};

} /* namespace smfs */

#endif /* INDEXEDPREFIXRULE_H_ */
