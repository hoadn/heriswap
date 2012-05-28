#include "SpawnGameStateManager.h"

#include <sstream>

#include "base/EntityManager.h"

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ADSRSystem.h"
#include "systems/ScrollingSystem.h"

#include "Game.h"
#include "DepthLayer.h"
#include "TwitchSystem.h"
#include "CombinationMark.h"


static void fillTheBlank(std::vector<Feuille>& spawning);
static Entity createCell(Feuille& f, bool assignGridPos);

SpawnGameStateManager::SpawnGameStateManager(SuccessManager* smgr){
	successMgr = smgr;
}

void SpawnGameStateManager::setAnimSpeed() {
	int difficulty = (theGridSystem.GridSize!=8)+1; //1 : normal, 2 : easy

	ADSR(eSpawn)->idleValue = 0;
	ADSR(eSpawn)->attackValue = 1.0;
	ADSR(eSpawn)->attackTiming = difficulty*0.2;
	ADSR(eSpawn)->decayTiming = 0;
	ADSR(eSpawn)->sustainValue = 1.0;
	ADSR(eSpawn)->releaseTiming = 0;

	ADSR(eGrid)->idleValue = 0;
	ADSR(eGrid)->attackValue = 1.0;
	ADSR(eGrid)->attackTiming = difficulty*0.3;
	ADSR(eGrid)->decayTiming = 0;
	ADSR(eGrid)->sustainValue = 1.0;
	ADSR(eGrid)->releaseTiming = 0;
}

void SpawnGameStateManager::Setup() {
	eSpawn = theEntityManager.CreateEntity();
	ADD_COMPONENT(eSpawn, ADSR);


	eGrid = theEntityManager.CreateEntity();
	ADD_COMPONENT(eGrid, ADSR);

	setAnimSpeed();
}

void SpawnGameStateManager::Enter() {
	LOGI("%s", __PRETTY_FUNCTION__);

	std::vector<Combinais> c;
	fillTheBlank(spawning);
	if ((int)spawning.size()==theGridSystem.GridSize*theGridSystem.GridSize) {
     	std::cout << "create " << spawning.size() << " cells" << std::endl;
		for(unsigned int i=0; i<spawning.size(); i++) {
            if (spawning[i].fe == 0)
			    spawning[i].fe = createCell(spawning[i], true);
		}
		int ite=0;
		//get a new grid which has no direct combinations but still combinations to do (give up at 100 try)
		do {
			c = theGridSystem.LookForCombination(false,true);
			// change type from cells in combi
			for(unsigned int i=0; i<c.size(); i++) {
				int j = MathUtil::RandomInt(c[i].points.size());
				Entity e = theGridSystem.GetOnPos(c[i].points[j].X, c[i].points[j].Y);
				Entity voisins[4] = {theGridSystem.GetOnPos(c[i].points[j].X+1, c[i].points[j].Y),
					theGridSystem.GetOnPos(c[i].points[j].X-1, c[i].points[j].Y),
					theGridSystem.GetOnPos(c[i].points[j].X, c[i].points[j].Y-1),
					theGridSystem.GetOnPos(c[i].points[j].X, c[i].points[j].Y+1)};
				int typeVoisins[4];
				for (int i= 0; i<4; i++)
					voisins[i] ? GRID(voisins[i])->type : -1;

				int type;
				do {
					type = MathUtil::RandomInt(theGridSystem.Types);
				} while (type == typeVoisins[0] || type == typeVoisins[1] || type == typeVoisins[2] || type == typeVoisins[3]);

				GRID(e)->type = type;
				RenderingComponent* rc = RENDERING(e);
				rc->texture = theRenderingSystem.loadTextureFile(Game::cellTypeToTextureNameAndRotation(type, &TRANSFORM(e)->rotation));
			}
			ite++;
		} while((!c.empty() || !theGridSystem.StillCombinations()) && ite<100);

        ADSR(eSpawn)->active = true;
	} else {
	    ADSR(eSpawn)->active = false;
    }
    ADSR(eSpawn)->activationTime = 0;

	ADSR(eGrid)->activationTime = 0;
	ADSR(eGrid)->active = false;
}

GameState SpawnGameStateManager::Update(float dt __attribute__((unused))) {
	ADSRComponent* transitionCree = ADSR(eSpawn);
	//si on doit recree des feuilles
	if (!spawning.empty()) {
        bool fullGridSpawn = (spawning.size() == (unsigned)theGridSystem.GridSize*theGridSystem.GridSize);
		transitionCree->active = true;
		for ( std::vector<Feuille>::reverse_iterator it = spawning.rbegin(); it != spawning.rend(); ++it ) {
			if (it->fe == 0) {
				it->fe = createCell(*it, fullGridSpawn);
			} else {
                GridComponent* gc = GRID(it->fe);
                if (fullGridSpawn) {
                    gc->i = gc->j = -1;
                }
				TransformationComponent* tc = TRANSFORM(it->fe);
				float s = Game::CellSize(theGridSystem.GridSize);
				if (transitionCree->value == 1){
					tc->size = Vector2(s*0.1, s);
					gc->i = it->X;
					gc->j = it->Y;
				} else {
					tc->size = Vector2(s * transitionCree->value, s * transitionCree->value);
				}
			}
		}
		if (transitionCree->value == 1) {
			spawning.clear();
			return NextState(true);
		}
	//sinon si on fait une nouvelle grille
	} else if (ADSR(eGrid)->active) {
        std::vector<Entity> feuilles = theGridSystem.RetrieveAllEntityWithComponent();
        for ( std::vector<Entity>::reverse_iterator it = feuilles.rbegin(); it != feuilles.rend(); ++it ) {
            Vector2 cellSize = Game::CellSize(theGridSystem.GridSize) * Game::CellContentScale() * (1 - ADSR(eGrid)->value);
            ADSR(*it)->idleValue = cellSize.X;
        }
        if (ADSR(eGrid)->value == ADSR(eGrid)->sustainValue) {
            theGridSystem.DeleteAll();
            fillTheBlank(spawning);
            LOGI("nouvelle grille de %lu elements! ", spawning.size());
            successMgr->gridResetted = true;
        }
    //sinon l'etat suivant depend de la grille actuelle
    } else {
		return NextState(false);
	}
	return Spawn;
}

GameState SpawnGameStateManager::NextState(bool marker) {
	std::vector<Combinais> combinaisons = theGridSystem.LookForCombination(false,marker);
	//si on a des combinaisons dans la grille on passe direct à Delete
	if (!combinaisons.empty()) {
		return Delete;
	//sinon
	} else {
		//si y a des combi, c'est au player de player
		if (theGridSystem.StillCombinations()) return UserInput;
		//sinon on genere une nouvelle grille
		else {
			ADSR(eGrid)->active = true;
			std::vector<Entity> feuilles = theGridSystem.RetrieveAllEntityWithComponent();
			for ( std::vector<Entity>::reverse_iterator it = feuilles.rbegin(); it != feuilles.rend(); ++it ) {
				CombinationMark::markCellInCombination(*it);
			}
		}
	}
	return Spawn;
}

void SpawnGameStateManager::Exit() {
	LOGI("%s", __PRETTY_FUNCTION__);
}

void fillTheBlank(std::vector<Feuille>& spawning)
{
	for (int i=0; i<theGridSystem.GridSize; i++){
		for (int j=0; j<theGridSystem.GridSize; j++){
			if (theGridSystem.GetOnPos(i,j) == 0){
				int r = MathUtil::RandomInt(theGridSystem.Types);
				Feuille nouvfe = {i,j,0,r};
				spawning.push_back(nouvfe);
			}
		}
	}
}

static Entity createCell(Feuille& f, bool assignGridPos) {
	Entity e = theEntityManager.CreateEntity(EntityManager::Persistent);
	ADD_COMPONENT(e, Transformation);
	ADD_COMPONENT(e, Rendering);
	ADD_COMPONENT(e, ADSR);
	ADD_COMPONENT(e, Grid);
    ADD_COMPONENT(e, Twitch);

	TRANSFORM(e)->position = Game::GridCoordsToPosition(f.X, f.Y, theGridSystem.GridSize);
	TRANSFORM(e)->z = DL_Cell;
	RenderingComponent* rc = RENDERING(e);
	rc->hide = false;

	TRANSFORM(e)->size = 0;
	ADSR(e)->idleValue = Game::CellSize(theGridSystem.GridSize) * Game::CellContentScale();
	GRID(e)->type = f.type;
	if (assignGridPos) {
		GRID(e)->i = f.X;
		GRID(e)->j = f.Y;
	}
	rc->texture = theRenderingSystem.loadTextureFile(Game::cellTypeToTextureNameAndRotation(f.type, &TRANSFORM(e)->rotation));
	return e;
}
