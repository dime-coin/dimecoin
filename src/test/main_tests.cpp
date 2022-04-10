// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <validation.h>
#include <net.h>
#include <reverse_iterator.h>

#include <test/test_dimecoin.h>

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

//! types to define test data in
typedef std::pair<int, CAmount> subsidyEntry;
std::vector<subsidyEntry> subsidyChecks;

void initSubsidyCheckData()
{
	subsidyChecks.push_back({100, 350000000});
	subsidyChecks.push_back({2351, 310272});
	subsidyChecks.push_back({3701, 644096});
	subsidyChecks.push_back({5051, 977920});
	subsidyChecks.push_back({6401, 263168});
	subsidyChecks.push_back({7751, 596992});
	subsidyChecks.push_back({9101, 930816});
	subsidyChecks.push_back({10451, 216064});
	subsidyChecks.push_back({11801, 549888});
	subsidyChecks.push_back({13151, 883712});
	subsidyChecks.push_back({14501, 168960});
	subsidyChecks.push_back({15851, 502784});
	subsidyChecks.push_back({17201, 836608});
	subsidyChecks.push_back({18551, 121856});
	subsidyChecks.push_back({19901, 455680});
	subsidyChecks.push_back({21251, 789504});
	subsidyChecks.push_back({22601, 74752});
	subsidyChecks.push_back({23951, 408576});
	subsidyChecks.push_back({25301, 742400});
	subsidyChecks.push_back({26651, 27648});
	subsidyChecks.push_back({28001, 361472});
	subsidyChecks.push_back({29351, 695296});
	subsidyChecks.push_back({30701, 1029120});
	subsidyChecks.push_back({32051, 314368});
	subsidyChecks.push_back({33401, 648192});
	subsidyChecks.push_back({34751, 982016});
	subsidyChecks.push_back({36101, 267264});
	subsidyChecks.push_back({37451, 601088});
	subsidyChecks.push_back({38801, 934912});
	subsidyChecks.push_back({40151, 220160});
	subsidyChecks.push_back({41501, 553984});
	subsidyChecks.push_back({42851, 887808});
	subsidyChecks.push_back({44201, 173056});
	subsidyChecks.push_back({45551, 506880});
	subsidyChecks.push_back({46901, 840704});
	subsidyChecks.push_back({48251, 125952});
	subsidyChecks.push_back({49601, 459776});
	subsidyChecks.push_back({50951, 793600});
	subsidyChecks.push_back({52301, 78848});
	subsidyChecks.push_back({53651, 412672});
	subsidyChecks.push_back({55001, 746496});
	subsidyChecks.push_back({56351, 31744});
	subsidyChecks.push_back({57701, 365568});
	subsidyChecks.push_back({59051, 699392});
	subsidyChecks.push_back({60401, 1033216});
	subsidyChecks.push_back({61751, 318464});
	subsidyChecks.push_back({63101, 652288});
	subsidyChecks.push_back({64451, 986112});
	subsidyChecks.push_back({65801, 271360});
	subsidyChecks.push_back({67151, 605184});
	subsidyChecks.push_back({68501, 939008});
	subsidyChecks.push_back({69851, 224256});
	subsidyChecks.push_back({71201, 558080});
	subsidyChecks.push_back({72551, 891904});
	subsidyChecks.push_back({73901, 177152});
	subsidyChecks.push_back({75251, 510976});
	subsidyChecks.push_back({76601, 844800});
	subsidyChecks.push_back({77951, 130048});
	subsidyChecks.push_back({79301, 463872});
	subsidyChecks.push_back({80651, 797696});
	subsidyChecks.push_back({82001, 82944});
	subsidyChecks.push_back({83351, 416768});
	subsidyChecks.push_back({84701, 750592});
	subsidyChecks.push_back({86051, 35840});
	subsidyChecks.push_back({87401, 369664});
	subsidyChecks.push_back({88751, 703488});
	subsidyChecks.push_back({90101, 1037312});
	subsidyChecks.push_back({91451, 322560});
	subsidyChecks.push_back({92801, 656384});
	subsidyChecks.push_back({94151, 990208});
	subsidyChecks.push_back({95501, 275456});
	subsidyChecks.push_back({96851, 609280});
	subsidyChecks.push_back({98201, 943104});
	subsidyChecks.push_back({99551, 228352});
	subsidyChecks.push_back({190001, 574464});
	subsidyChecks.push_back({460001, 230400});
	subsidyChecks.push_back({585551, 433664});
	subsidyChecks.push_back({730001, 467456});
	subsidyChecks.push_back({1000001, 295424});
	subsidyChecks.push_back({1270001, 61696});
	subsidyChecks.push_back({1540001, 118912});
	subsidyChecks.push_back({1810001, 75904});
	subsidyChecks.push_back({2000351, 61312});
	subsidyChecks.push_back({2065151, 49088});
	subsidyChecks.push_back({2080001, 16448});
	subsidyChecks.push_back({2300051, 9408});
	subsidyChecks.push_back({2350001, 60480});
	subsidyChecks.push_back({2620001, 19488});
	subsidyChecks.push_back({2644301, 10656});
	subsidyChecks.push_back({2645651, 21088});
	subsidyChecks.push_back({2647001, 31520});
	subsidyChecks.push_back({2648351, 9184});
	subsidyChecks.push_back({2649701, 19616});
	subsidyChecks.push_back({2890001, 8736});
	subsidyChecks.push_back({3000701, 12192});
	subsidyChecks.push_back({3160001, 15376});
	subsidyChecks.push_back({3296351, 1520});
	subsidyChecks.push_back({3430001, 8192});
	subsidyChecks.push_back({3700001, 8192});
	subsidyChecks.push_back({3970001, 7536});
	subsidyChecks.push_back({4240001, 7536});
	subsidyChecks.push_back({4510001, 6933});
	subsidyChecks.push_back({4780001, 6933});
	subsidyChecks.push_back({4838051, 6933});
	subsidyChecks.push_back({5000051, 6379});
	subsidyChecks.push_back({5050001, 6379});
	subsidyChecks.push_back({5320001, 6379});
	subsidyChecks.push_back({5590001, 5868});
	subsidyChecks.push_back({5784401, 5868});
	subsidyChecks.push_back({5860001, 5868});
	subsidyChecks.push_back({6000401, 5399});
	subsidyChecks.push_back({6130001, 5399});
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
	initSubsidyCheckData();
	const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
	CAmount nSum = 0;
	for (auto l : subsidyChecks) {
		CAmount nSubsidy = GetBlockSubsidy(l.first, chainParams->GetConsensus());
		BOOST_CHECK_EQUAL(l.second, nSubsidy / COIN);
		nSum += nSubsidy;
		BOOST_CHECK(MoneyRange(nSum));
	}
	BOOST_CHECK_EQUAL(nSum, CAmount{39176226390769});
}

static bool ReturnFalse() { return false; }
static bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool (), CombinerAll> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}
BOOST_AUTO_TEST_SUITE_END()
