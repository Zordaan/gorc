#include <nullunit/nullunit.h>

#include "contenttest/loader_test_fixture.h"
#include "contenttest/vfssingleton.h"

#include "content/assets/level.h"

class JklTestFixture : public LoaderTestFixture {
public:
	JklTestFixture() : LoaderTestFixture(VfsSingleton::get()) {
		return;
	}
};

BeginSuiteFixture(JklTest, JklTestFixture);

Case(jk_01narshadda_test) {
	VfsSingleton::SetEpisode("The Force Within");
	auto lev = TryLoad<gorc::content::assets::level>("01narshadda.jkl", compiler);

	AssertResult(2, 539);
}

EndSuite(JklTest);
