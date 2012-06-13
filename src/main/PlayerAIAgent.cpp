#include "PlayerAIAgent.h"
#include "AIDivideAreaManager.h"
#include "Monster.h"
#include <Logic/RaycastComponent.hpp>

const QString PlayerAIAgent::INTERACTOR_COMPONENT = "Player_AI_Agent_Interactor";
const QString PlayerAIAgent::TRIGGER_AREA_COMPONENT = "Player_AI_TRIGGER_AREA_COMPONENT";
const double  PlayerAIAgent::THREAT_COOL_TIME = 2.0;
const double  PlayerAIAgent::eps = 1e-4;
const double  PlayerAIAgent::MOVE_ROTATE_SPEED = 360;
const double  PlayerAIAgent::GUARD_ROTATE_SPEED = 180;
const double  PlayerAIAgent::PI = acos(-1.0);
const double  PlayerAIAgent::ROTATE_FLOAT = PI / 24; 
const double  PlayerAIAgent::ENTER_SCOPE = 1.0;

PlayerAIAgent::PlayerAIAgent(QString name): Agent(name) {    
    //��ʼ�����κ�״̬
    mFollow = mThreat = mOnWay = false; 
}

bool PlayerAIAgent::isOnWay() {
	return mOnWay;
}

void PlayerAIAgent::setOnWay(bool type) {
	mOnWay = type;
}

Alien* PlayerAIAgent::getBody() {
	return mBody;
}

void PlayerAIAgent::setBody(Alien * body) {
	mBody = body;
}
void PlayerAIAgent::walk(double time_diff) {   
   //[-180,180]
    double pre_degree;
    double cur_degree;
    double expect_degree;
    Ogre::Vector3 tmp;
    Ogre::Degree td; 
    mBody->getRotation().ToAngleAxis(td, tmp);
    pre_degree =td.valueDegrees() * tmp.y;
    
    Ogre::Vector3 dy = Ogre::Vector3(0, 0, 1); 
    Ogre::Vector3 nxt_area_position = AIDivideAreaManager::get()->getPositionById(mNxtArea);    
    Ogre::Vector3 pre_position = mBody->getPosition(); 
   
    Ogre::Vector3 dv = nxt_area_position - pre_position; 
    dv.y = 0; 
    
    expect_degree = asin((double) ( dy.crossProduct(dv).y / (dy.length() * dv.length()) )) * 360 / PI;
    
    double d_degree = expect_degree - pre_degree;
    if (fabs(d_degree) > 180 + eps) {
        if (d_degree < 0) d_degree = 360 + d_degree;
        else d_degree = d_degree - 360;
    }

    //��ǰ֡����Ѿ��ڽǶȷ����ڣ���ʼ�߶���
    if (fabs(d_degree) < ROTATE_FLOAT) {
        emit(sMove(Entity::FORWARD, true)); 
        Ogre::Vector3 cur_position = mBody->getPosition(); 
        //�Ѿ�����Ŀ�꣬mOnWay״̬ȡ����
        if (nxt_area_position.distance(cur_position) < ENTER_SCOPE) {
            mOnWay = false;             
        }
    } else {        
        emit(sMove(Entity::STOP, true));
        if (d_degree > 0) 
              emit(sLookAround(Ogre::Quaternion(Ogre::Degree(pre_degree + MOVE_ROTATE_SPEED * time_diff),
                                                Ogre::Vector3(0,1,0))));
        else  emit(sLookAround(Ogre::Quaternion(Ogre::Degree(pre_degree - MOVE_ROTATE_SPEED * time_diff),
                                                Ogre::Vector3(0,1,0))));
    }
    
}

void PlayerAIAgent::guard(double time_diff) {
    //����ǰ����û�����࣬�о�ͳͳ����
    this->findComponent<dt::InteractionComponent>(INTERACTOR_COMPONENT)->check();

    if (!mHasEnemy) {
        Ogre::Vector3 tmp;
        Ogre::Degree td; 
        mBody->getRotation().ToAngleAxis(td, tmp);
        double cur_degree = td.valueDegrees();
        emit(sLookAround(Ogre::Quaternion(Ogre::Degree(cur_degree + time_diff * GUARD_ROTATE_SPEED),
                                          Ogre::Vector3(0,1,0))));

    }
    mHasEnemy = false; 
}
void PlayerAIAgent::decision(double time_diff) {

}
void PlayerAIAgent::onUpdate(double time_diff) {
    //����в������֮��
    if (mThreat) {
        mThreatTime -= time_diff; 
        if (mThreatTime <= eps) {
            mThreat = false; 
        }
        guard(time_diff); 
    } else if (mOnWay) { //�����ߣ�����֮��
        walk(time_diff); 
    } else decision(time_diff);  // ���򣬾���֮��
}

void PlayerAIAgent::initialize() {

    setBody(dynamic_cast<Alien *>(this->getParent()));

    auto iteractor = this->addComponent<dt::InteractionComponent>(
        new dt::RaycastComponent(INTERACTOR_COMPONENT));
    //ֻҪû���ϰ���Ϳ��Թ�����
    iteractor->setRange(3000.0f);    
    connect(iteractor.get(), SIGNAL(sHit(dt::PhysicsBodyComponent*)),
            this, SLOT(__onFire(dt::PhysicsBodyComponent*)));

    auto trigger = this->addComponent<dt::TriggerAreaComponent>(new dt::TriggerAreaComponent(
        new btBoxShape(btVector3(5.0f, 5.0f, 5.0f)), TRIGGER_AREA_COMPONENT));
    connect(trigger.get(), SIGNAL(triggered(dt::TriggerAreaComponent*, dt::Component*)), 
            this, SLOT(__onTrigger(dt::TriggerAreaComponent*, dt::Component*)));
}


void PlayerAIAgent::__onFire(dt::PhysicsBodyComponent* pbc) {
     Monster* enemy = dynamic_cast<Monster*>(pbc->getNode());
     if (enemy != nullptr) {
        emit(sAttack(true));
        mHasEnemy = true; 
     } else {
        emit(sAttack(false));        
     }
}

void PlayerAIAgent::__onTrigger(dt::TriggerAreaComponent* tac, dt::Component* c) {
    Monster* enemy = dynamic_cast<Monster*>(c->getNode());
    if (enemy != nullptr) {
        mThreat = true; 
        mThreatTime = THREAT_COOL_TIME; 
        return; 
    }
    //����!
    Alien * gun_friend = dynamic_cast<Alien *>(c->getNode());
    if (gun_friend != nullptr) {
       // gun_friend->get
    }
    
}