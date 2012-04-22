#include <sstream>

#include "base/EntityManager.h"
#include "base/MathUtil.h"

#include "systems/RenderingSystem.h"

#include "AnimedEntity.h"

void switchAnim(AnimatedActor* a) {
	a->frames++;
	if (a->frames>=30/(MathUtil::Abs(a->actor.speed))) {
		RENDERING(a->actor.e)->texture = theRenderingSystem.loadTextureFile(a->anim[a->ind]);
		a->ind++;
		if (a->ind==a->anim.size()) a->ind = 0;
		a->frames=0;
	}
}

void loadHerissonTexture(int type, AnimatedActor* c) {
	std::stringstream t;
	c->anim.clear();
	t << "herisson_1_"<<type<<".png";
	c->anim.push_back(t.str());
	t.str("");
	t << "herisson_2_"<<type<<".png";
	c->anim.push_back(t.str());
	t.str("");
	t << "herisson_3_"<<type<<".png";
	c->anim.push_back(t.str());
	t.str("");
	t << "herisson_2_"<<type<<".png";
	c->anim.push_back(t.str());
}