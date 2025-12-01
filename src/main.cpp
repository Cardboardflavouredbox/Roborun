// Copyright 2025 Greenbox
#include <SFML/Graphics.hpp>
#include <glaze/glaze.hpp>
#include <deque>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <cmath>
sf::RenderTexture rt({160, 144});//랜더텍스쳐 (화면에 그릴거 있으면 여기)
sf::RenderTexture uirt({160,144});//
auto window = sf::RenderWindow(sf::VideoMode({160, 144}), "Roborun");
char right = 0, down = 0, up = 0, left = 0, confirm = 0, cancel = 0, select = 0, start = 0, save = 0, leftclick = 0, rightclick = 0, esc = 0;//2=이번 프레임에 누름, 1=꾹 누르고 있음, 0=안 누르고 있음.
sf::Keyboard::Key rightkey = sf::Keyboard::Key::Right,
                  downkey = sf::Keyboard::Key::Down,
                  upkey = sf::Keyboard::Key::Up,
                  leftkey = sf::Keyboard::Key::Left,
                  confirmkey = sf::Keyboard::Key::Z,
                  cancelkey = sf::Keyboard::Key::X,
                  startkey = sf::Keyboard::Key::Enter,
                  selectkey = sf::Keyboard::Key::C,
                  savekey = sf::Keyboard::Key::S,
                  esckey = sf::Keyboard::Key::Escape;
sf::View view({0.f, 0.f}, {160.f, 144.f});
bool groundcheck=false;//땅에 닿았는지 여부
bool doublejump=true;//더블점프 가능 여부
bool dash=true;//대시 가능 여부
bool floating=false;//호버링 하고 있는지
bool debug=false;//디버그 모드
bool gettinghit=false;//데미지 에니메이션
char hp=3;//체력
char iframes=0;//무적프레임
char attacking=0;//공격 양수일시 가로, 음수일시 세로
float screensizex=1,screensizey=1;
bool mainmenu=true;//메인 메뉴인가?
float mainmenulerp=0;//메인 메뉴 에니메이션
char mainmenuoption=0;//메인 메뉴 선택
char hitstop=0;//히트스탑
int deathanim=60;
bool placetypeground=true;
std::unordered_map<std::string,sf::Texture> texturemap;//텍스쳐맵

float Lerp(float A, float B, float Alpha) {//선형 보간 함수
  return A * (1 - Alpha) + B * Alpha;
}

class Particle : public sf::Drawable, public sf::Transformable
{
public:
    void set(unsigned int size,unsigned int length,unsigned int codething,float direction,float speedthing,sf::PrimitiveType type,sf::Color color){
      va.resize(size);
      va.setPrimitiveType(type);
      for(unsigned int i=0;i<size;i++)va[0].color=color;
      frame=0;
      len=length;
      code=codething;
      dir=direction;
      speed=speedthing;
    }
    void update(){
      switch(code){
        case 0:{//basic particle
            va[0].position=sf::Vector2f(std::cos(dir * 3.14 / 180) * frame * speed,std::sin(dir * 3.14 / 180) * frame * speed);
        }
      }
      frame++;
    }
    unsigned int frame,len,code;

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        states.transform *= getTransform();
        states.texture = nullptr;
        target.draw(va, states);
    }
    sf::VertexArray va;
    float dir,speed;
};
std::deque<Particle> particledeque;

struct ground{//땅 클래스
    int x,y,x2,y2;
    bool operator< (const ground& temp) const {return x < temp.x;}
};
struct obstacle{//장애물 클래스
    int x,y,x2,y2;
    bool destroyable=false;
    bool bouncable=true;
    bool destroyed=false;
    bool operator< (const obstacle& temp) const {return x < temp.x;}
};
struct map{
  std::deque<ground> grounddeque;//땅의 데이터가 저장된 덱
  std::deque<obstacle> obstacledeque;//장애물의 데이터가 저장된 덱
};
map currentmap,tempmap,loadedmap;

struct entity{//엔티티 클래스
    int x=0,y=0;
    float xvelocity=0;//X 속도
    float yvelocity=0;//Y 속도
    int vertx=-8,verty=-1,vertx2=8,verty2=0;
    int hitboxx1=-8,hitboxy1=-16,hitboxx2=8,hitboxy2=0;
    int anim=0;
};
entity player;//플레이어
entity debugdelete;//삭제판정
entity attack;//공격판정

void debugset(){
  tempmap=currentmap;
  loadedmap=currentmap;
}

void setgame(){//게임세팅 함수
  tempmap=currentmap;
  player.x=0;player.y=0;
  player.yvelocity=0;
  view.setCenter({float(player.x+64),-64});
  player.xvelocity=2;
  groundcheck=false;
  doublejump=true;
  dash=true;
  floating=false;
  debug=false;
  gettinghit=false;
  hp=3;
  iframes=0;
  attacking=0;
  hitstop=0;
  deathanim=60;
}

bool overlap(entity p,ground g)//엔티티와 땅이 겹치는지 확인하는 함수
{
   if (p.hitboxx1+p.x >= g.x+g.x2 || g.x >= p.hitboxx2+p.x )
        return false;

    if (p.hitboxy2+p.y  <= g.y || g.y2+g.y <= p.hitboxy1+p.y)
        return false;

    return true;
}

bool overlap(entity p,obstacle g)//엔티티와 땅이 겹치는지 확인하는 함수
{
   if (p.hitboxx1+p.x >= g.x+g.x2 || g.x >= p.hitboxx2+p.x )
        return false;

    if (p.hitboxy2+p.y  <= g.y || g.y2+g.y <= p.hitboxy1+p.y)
        return false;

    return true;
}

void windowset(){//윈도우 설정용 함수
  while (std::optional event = window.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      window.close();
    }

    if (auto resized = event->getIf<sf::Event::Resized>()) {
      float x, y;
      if ((resized->size.x) > (resized->size.y)) {
        y = 144;
        x = (float(resized->size.x) / float(resized->size.y) * 144.f);
      } else {
        x = 160;
        y = (float(resized->size.y) / float(resized->size.x) * 160.f);
      }
      sf::FloatRect visibleArea({(-x + 160.f) / 2, (-y + 144.f) / 2}, {x, y});
      window.setView(sf::View(visibleArea));
      screensizex=window.getSize().x/160.f;
      screensizey=window.getSize().y/144.f;
    }
  }
}

void keypresscheck(sf::Keyboard::Key keycode, char* key) {//키 인식 함수
  if (sf::Keyboard::isKeyPressed(keycode)) {
    if (*key == 0)
      *key = 2;//2=지금 눌렀을때
    else if (*key == 2)
      *key = 1;//1=꾹 누르고 있을때
  } else {
    *key = 0;//0=아예 안 누를때
  }
}

void groundcollisioncheck(){//땅 인식 함수
  groundcheck=false;
  player.y++;
  for(int i=0;i<loadedmap.grounddeque.size();i++){
    if(overlap(player,loadedmap.grounddeque[i])){
      groundcheck=true;
      while(overlap(player,loadedmap.grounddeque[i]))player.y--;
      break;
      }
  }
  if(!groundcheck)player.y--;
}

void obstaclecollisioncheck(){//장애물 인식 함수
  for(int i=0;i<loadedmap.obstacledeque.size();i++){
    if(!loadedmap.obstacledeque[i].destroyed&&overlap(player,loadedmap.obstacledeque[i])){
      hp--;
      player.xvelocity=-2;
      player.yvelocity=0;
      iframes=96;
      hitstop=7;
      gettinghit=true;
      break;
      }
  }
}

void attackcollisioncheck(entity temp){//장애물 공격 인식 함수
  for(int i=0;i<loadedmap.obstacledeque.size();i++){
    if(!loadedmap.obstacledeque[i].destroyed&&overlap(temp,loadedmap.obstacledeque[i])){
      if(loadedmap.obstacledeque[i].destroyable)loadedmap.obstacledeque[i].destroyed=true;//파괴 가능 장애물일시 파괴
      if(attacking>0){
        hitstop=5;
        player.xvelocity=-2;//가로 공격 적중 시 뒤 밀쳐짐
      }
      else if(loadedmap.obstacledeque[i].bouncable){//튀어오를 수 있는 장애물일시
        hitstop=5;
        player.yvelocity=-5;//세로 공격 적중 시 튀어오름
        doublejump=true;//더블점프 활성화
        dash=true;//대시 활성화
      }
      break;
      }
  }
}

void deletecheck(){
  for(int i=0;i<loadedmap.grounddeque.size();i++){
    if(overlap(debugdelete,loadedmap.grounddeque[i])){
      loadedmap.grounddeque.erase(loadedmap.grounddeque.begin()+i);
      i--;
    }
  }
  for(int i=0;i<loadedmap.obstacledeque.size();i++){
    if(overlap(debugdelete,loadedmap.obstacledeque[i])){
      loadedmap.obstacledeque.erase(loadedmap.obstacledeque.begin()+i);
      i--;
    }
  }
  debugdelete.hitboxx1=0;debugdelete.hitboxx2=0;
  debugdelete.hitboxy1=0;debugdelete.hitboxy2=0;
}

void mainmenuupdate(){
  mainmenulerp=Lerp(mainmenulerp,0,0.1f);
  if(right==2&&mainmenuoption==0){mainmenuoption=1;mainmenulerp=50;}
  else if(left==2&&mainmenuoption==1){mainmenuoption=0;mainmenulerp=50;}
  if(confirm==2){
    mainmenu=false;
    if(mainmenuoption==1){debug=true;debugset();}
    else debug=false;
  }
}


void gameloopupdate() {
  view.setCenter({Lerp(view.getCenter().x,float(player.x+64),0.5f),-64});
  
  if(!particledeque.empty()&&particledeque.front().frame>=particledeque.front().len)particledeque.pop_front();
  for(int i=0;i<particledeque.size();i++){
    particledeque[i].update();
  }
  

  if(!tempmap.grounddeque.empty()&&tempmap.grounddeque.front().x<view.getCenter().x+160){
    loadedmap.grounddeque.push_back(tempmap.grounddeque.front());
    tempmap.grounddeque.pop_front();
  }
  if(!tempmap.obstacledeque.empty()&&tempmap.obstacledeque.front().x<view.getCenter().x+160){
    loadedmap.obstacledeque.push_back(tempmap.obstacledeque.front());
    tempmap.obstacledeque.pop_front();
  }
  if(!loadedmap.grounddeque.empty()&&loadedmap.grounddeque.back().x+loadedmap.grounddeque.back().x2<view.getCenter().x-160)loadedmap.grounddeque.pop_back();
  if(!loadedmap.obstacledeque.empty()&&loadedmap.obstacledeque.back().x+loadedmap.obstacledeque.back().x2<view.getCenter().x-160)loadedmap.obstacledeque.pop_back();

  groundcollisioncheck();

  if(groundcheck){player.yvelocity=0;doublejump=true;}//더블 점프 활성화 + 땅에 있을시에 y가속도 0으로 설정
  else if(player.xvelocity<=2) player.yvelocity+=0.5f*(floating&&player.yvelocity>0?0.125f:1);//중력

  if(player.xvelocity>2)player.xvelocity-=0.5f;//X속도가 2보다 크다면 0.5씩 감소

  if(attacking>0)attacking--;
  else if(attacking<0)attacking++;

  if(player.xvelocity<2)player.xvelocity+=0.25f;
  if(player.xvelocity==2&&groundcheck)dash=true;
  if(confirm==2&&player.xvelocity<=2){//점프 키를 눌렀을 시
    floating=false;
    if(groundcheck){player.yvelocity=-7;groundcheck=false;}//땅에 닿았을 시 점프
    else if(doublejump){//땅에 닿지 않았을시 더블 점프가 가능하다면 더블 점프
      doublejump=false;
      player.yvelocity=-7;
      Particle temp;
      temp.set(1,30,0,90,0.5f,sf::PrimitiveType::Points,sf::Color::White);
      for(int i=0;i<4;i++){
        temp.setPosition({float(player.x+8-i*4),float(player.y)});
        particledeque.push_back(temp);
      }
    }
  }
  else if(confirm==1){//점프 키를 꾹 눌렀을 시
    floating=true;
  }
  else floating=false;
  if(select==2&&dash){//대시 가능하고 셀렉트 키(C)를 눌렀을 시
    floating=false;
    dash=false;
    player.xvelocity=10;
    player.yvelocity=0;
    Particle temp;
    temp.set(1,30,0,180,0.5f,sf::PrimitiveType::Points,sf::Color::White);
    for(int i=0;i<3;i++){
      temp.setPosition({float(player.x),float(player.y-i*4)});
      particledeque.push_back(temp);
    }
  }
  if(cancel==2&&player.xvelocity<=2){//공격 키를 눌렀을 시
    floating=false;
    if(groundcheck)attacking=10;//공격 변수 2로 설정 (양수일시 가로 공격)
    else attacking=-10;//공격 변수 -2로 설정 (음수일시 세로 공격)
  }
  if(attacking>6&&attacking<8){
    attack={player.x,player.y,0,0,0,0,0,0,4,-16,32,0};//공격 판정 엔티티
    attackcollisioncheck(attack);//장애물과 공격의 충돌 확인 함수
  }
  else if(attacking<-6&&attacking>-8){
    attack={player.x,player.y,0,0,0,0,0,0,-12,-8,12,24};//공격 판정 엔티티
    attackcollisioncheck(attack);//장애물과 공격의 충돌 확인 함수
  }
  groundcollisioncheck();
  player.x+=player.xvelocity;
  if(player.yvelocity<0)player.y+=player.yvelocity;
  else for(int i=0;i<player.yvelocity;i++){
    player.y++;
    groundcollisioncheck();
    if(groundcheck){player.yvelocity=0;doublejump=true;}
  }

  if(iframes>0)iframes--;
  else obstaclecollisioncheck();

  if (player.y > 64) {
    hp=0;
  }
}

void gameoverupdate(){
  if(deathanim>0){
    iframes=0;
    view.setCenter({Lerp(view.getCenter().x,float(player.x),0.5f),Lerp(view.getCenter().y,float(player.y),0.5f)});
    deathanim--;
    if(deathanim==0)player.yvelocity=-4;
  }
  else{
    player.y+=player.yvelocity;
    player.yvelocity+=0.25f;
    if(player.yvelocity>16){
      mainmenu=true;
      setgame();
    }
  }
}

void update(){
  if(hitstop>0){
    hitstop--;
    if(hitstop==0)attacking=0;
    }
  else{
    gettinghit=false;
    if(mainmenu)mainmenuupdate();
    else if(hp>0)gameloopupdate();
    else gameoverupdate();
  }
}

void debugupdate(){
  if(confirm==2&&leftclick==0&&rightclick==0)placetypeground=!placetypeground;
  if(right==2)view.setCenter(view.getCenter()+sf::Vector2f(40,0));
  if(left==2)view.setCenter(view.getCenter()+sf::Vector2f(-40,0));
  if(save==2){
    std::sort(loadedmap.grounddeque.begin(),loadedmap.grounddeque.end());
    std::sort(loadedmap.obstacledeque.begin(),loadedmap.obstacledeque.end());
    for(int i=0;i<loadedmap.grounddeque.size();i++){
      if(loadedmap.grounddeque[i].x2<0){loadedmap.grounddeque[i].x+=loadedmap.grounddeque[i].x2;loadedmap.grounddeque[i].x2*=-1;}
      if(loadedmap.grounddeque[i].y2<0){loadedmap.grounddeque[i].y+=loadedmap.grounddeque[i].y2;loadedmap.grounddeque[i].y2*=-1;}
    }
    for(int i=0;i<loadedmap.obstacledeque.size();i++){
      if(loadedmap.obstacledeque[i].x2<0){loadedmap.obstacledeque[i].x+=loadedmap.obstacledeque[i].x2;loadedmap.obstacledeque[i].x2*=-1;}
      if(loadedmap.obstacledeque[i].y2<0){loadedmap.obstacledeque[i].y+=loadedmap.obstacledeque[i].y2;loadedmap.obstacledeque[i].y2*=-1;}
    }
    auto error=glz::write_file_json(loadedmap,"assets/database/map.json",std::string{});
    if(error){
      std::string error_msg = glz::format_error(error, "assets/database/map.json");
      std::cout << error_msg << '\n';
    }
  }
  if (leftclick==2){
    if(placetypeground){
      ground temp;
      temp.x = ((int)(((sf::Mouse::getPosition(window).x)/screensizey+view.getCenter().x)-80)/(int)8)*8;
      temp.y = ((int)(((sf::Mouse::getPosition(window).y)/screensizex+view.getCenter().y)-72)/(int)8)*8;
      temp.x2 = 0;
      temp.y2 = 0;
      loadedmap.grounddeque.push_back(temp);
    }
    else{
      obstacle temp;
      temp.x = ((int)(((sf::Mouse::getPosition(window).x)/screensizey+view.getCenter().x)-80)/(int)8)*8;
      temp.y = ((int)(((sf::Mouse::getPosition(window).y)/screensizex+view.getCenter().y)-72)/(int)8)*8;
      temp.x2 = 0;
      temp.y2 = 0;
      loadedmap.obstacledeque.push_back(temp);
    }
  }
  else if(leftclick==1){
    if(placetypeground){
      loadedmap.grounddeque.back().x2 = ((int)(((sf::Mouse::getPosition(window).x)/screensizey+view.getCenter().x)-80)/(int)8)*8-loadedmap.grounddeque.back().x;
      loadedmap.grounddeque.back().y2 = 1;
    }
    else{
      loadedmap.obstacledeque.back().x2 = ((int)(((sf::Mouse::getPosition(window).x)/screensizey+view.getCenter().x)-80)/(int)8)*8-loadedmap.obstacledeque.back().x;
      loadedmap.obstacledeque.back().y2 = ((int)(((sf::Mouse::getPosition(window).y)/screensizex+view.getCenter().y)-72)/(int)8)*8-loadedmap.obstacledeque.back().y;
    }
  }
  else if(rightclick==2){
    debugdelete.x = ((int)(((sf::Mouse::getPosition(window).x)/screensizey+view.getCenter().x)-80)/(int)8)*8;
    debugdelete.y = ((int)(((sf::Mouse::getPosition(window).y)/screensizex+view.getCenter().y)-72)/(int)8)*8;
    debugdelete.hitboxx1=0;debugdelete.hitboxy1=0;
    debugdelete.hitboxx2=0;debugdelete.hitboxy2=0;
  }
  else if(rightclick==1){
    debugdelete.hitboxx2 = ((int)(((sf::Mouse::getPosition(window).x)/screensizey+view.getCenter().x)-80)/(int)8)*8-debugdelete.x;
    debugdelete.hitboxy2 = ((int)(((sf::Mouse::getPosition(window).y)/screensizex+view.getCenter().y)-72)/(int)8)*8-debugdelete.y;
  }
  
  if(rightclick==0&&(debugdelete.hitboxx1!=0||debugdelete.hitboxx2!=0||debugdelete.hitboxy1!=0||debugdelete.hitboxy2!=0)){
    deletecheck();
  }
  if(leftclick==0&&!loadedmap.grounddeque.empty()&&loadedmap.grounddeque.back().x2==loadedmap.grounddeque.back().x)loadedmap.grounddeque.pop_back();
  if(leftclick==0&&!loadedmap.obstacledeque.empty()&&loadedmap.obstacledeque.back().x2==loadedmap.obstacledeque.back().x)loadedmap.obstacledeque.pop_back();

  if(esc==2){
    mainmenu=true;
    setgame();
  }
}

void input(){//입력을 받는 함수 (건드리지 않는게 좋음)
  keypresscheck(rightkey,&right);
  keypresscheck(leftkey,&left);
  keypresscheck(upkey,&up);
  keypresscheck(downkey,&down);
  keypresscheck(confirmkey,&confirm);
  keypresscheck(cancelkey,&cancel);
  keypresscheck(startkey,&start);
  keypresscheck(selectkey,&select);
  keypresscheck(savekey,&save);
  keypresscheck(esckey,&esc);
  if(sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
    if(leftclick==0)leftclick=2;
    else leftclick=1;
  }
  else leftclick=0;
  if(sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)){
    if(rightclick==0)rightclick=2;
    else rightclick=1;
  }
  else rightclick=0;
}

void mainmenurender(){
  sf::VertexArray tri(sf::PrimitiveType::TriangleStrip, 4);
  tri[0].position={0,0};
  tri[1].position={160,0};
  tri[2].position={0,144};
  tri[3].position={160,144};
  tri[0].texCoords={0,0};
  tri[1].texCoords={160,0};
  tri[2].texCoords={0,144};
  tri[3].texCoords={160,144};
  uirt.draw(tri,&texturemap["Background1"]);


  sf::Sprite sprite(texturemap["Options"]);

  sprite.setTextureRect({{0,0},{64,32}});
  sprite.setOrigin({32,16});
  sprite.setPosition({48,112});
  if(mainmenuoption==1){sprite.setScale({(50+mainmenulerp)/100,(50+mainmenulerp)/100});}
  else{sprite.setScale({(100-mainmenulerp)/100,(100-mainmenulerp)/100});}
  uirt.draw(sprite);
  sprite.setPosition({112,112});
  sprite.setTextureRect({{0,32},{64,64}});
  if(mainmenuoption==0){sprite.setScale({(50+mainmenulerp)/100,(50+mainmenulerp)/100});}
  else sprite.setScale({(100-mainmenulerp)/100,(100-mainmenulerp)/100});
  uirt.draw(sprite);
}

void gamelooprender() {//메인 게임 랜더 함수
  sf::VertexArray tri(sf::PrimitiveType::TriangleStrip, 4);
  if(hp>0){
    for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
    tri[0].position=sf::Vector2f(view.getCenter().x-80-(player.x/2)%160,-72-64);
    tri[1].position=sf::Vector2f(view.getCenter().x+80-(player.x/2)%160,-72-64);
    tri[2].position=sf::Vector2f(view.getCenter().x-80-(player.x/2)%160,72-64);
    tri[3].position=sf::Vector2f(view.getCenter().x+80-(player.x/2)%160,72-64);
    tri[0].texCoords=sf::Vector2f(0,0);
    tri[1].texCoords=sf::Vector2f(160,0);
    tri[2].texCoords=sf::Vector2f(0,144);
    tri[3].texCoords=sf::Vector2f(160,144);
    for(int i=0;i<4;i++)tri[i].position+=sf::Vector2f(-160,0);
    rt.draw(tri,&texturemap["Background1"]);
    for(int i=0;i<4;i++)tri[i].position+=sf::Vector2f(160,0);
    rt.draw(tri,&texturemap["Background1"]);
    for(int i=0;i<4;i++)tri[i].position+=sf::Vector2f(160,0);
    rt.draw(tri,&texturemap["Background1"]);
    for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
    for(int i=0;i<loadedmap.grounddeque.size();i++){
      tri[0].position=sf::Vector2f(loadedmap.grounddeque[i].x,loadedmap.grounddeque[i].y);
      tri[1].position=sf::Vector2f(loadedmap.grounddeque[i].x+loadedmap.grounddeque[i].x2,loadedmap.grounddeque[i].y);
      tri[2].position=sf::Vector2f(loadedmap.grounddeque[i].x,loadedmap.grounddeque[i].y+loadedmap.grounddeque[i].y2);
      tri[3].position=sf::Vector2f(loadedmap.grounddeque[i].x+loadedmap.grounddeque[i].x2,loadedmap.grounddeque[i].y+loadedmap.grounddeque[i].y2);
      rt.draw(tri);
    }
    for(int i=0;i<4;i++)tri[i].color=sf::Color::Red;
    for(int i=0;i<loadedmap.obstacledeque.size();i++){
      tri[0].position=sf::Vector2f(loadedmap.obstacledeque[i].x,loadedmap.obstacledeque[i].y);
      tri[1].position=sf::Vector2f(loadedmap.obstacledeque[i].x+loadedmap.obstacledeque[i].x2,loadedmap.obstacledeque[i].y);
      tri[2].position=sf::Vector2f(loadedmap.obstacledeque[i].x,loadedmap.obstacledeque[i].y+loadedmap.obstacledeque[i].y2);
      tri[3].position=sf::Vector2f(loadedmap.obstacledeque[i].x+loadedmap.obstacledeque[i].x2,loadedmap.obstacledeque[i].y+loadedmap.obstacledeque[i].y2);
      rt.draw(tri);
    }
    

    if(attacking>0){
      for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
      tri[0].position=sf::Vector2f(player.x,player.y-20);
      tri[1].position=sf::Vector2f(player.x+32,player.y-20);
      tri[2].position=sf::Vector2f(player.x,player.y+12);
      tri[3].position=sf::Vector2f(player.x+32,player.y+12);

      int tempanim=0;
      if(attacking<6)tempanim=2;
      else if(attacking>8)tempanim=0;
      else tempanim=1;
      tri[0].texCoords=sf::Vector2f(tempanim*32+0,0);
      tri[1].texCoords=sf::Vector2f(tempanim*32+32,0);
      tri[2].texCoords=sf::Vector2f(tempanim*32+0,32);
      tri[3].texCoords=sf::Vector2f(tempanim*32+32,32);
      rt.draw(tri,&texturemap["Slash"]);
    }
    else if(attacking<0){
      for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
      tri[2].position=sf::Vector2f(player.x-16,player.y);
      tri[0].position=sf::Vector2f(player.x+16,player.y);
      tri[3].position=sf::Vector2f(player.x-16,player.y+24);
      tri[1].position=sf::Vector2f(player.x+16,player.y+24);

      int tempanim=0;
      if(attacking>-6)tempanim=2;
      else if(attacking<-8)tempanim=0;
      else tempanim=1;
      tri[0].texCoords=sf::Vector2f(tempanim*32+0,0);
      tri[1].texCoords=sf::Vector2f(tempanim*32+32,0);
      tri[2].texCoords=sf::Vector2f(tempanim*32+0,32);
      tri[3].texCoords=sf::Vector2f(tempanim*32+32,32);
      rt.draw(tri,&texturemap["Slash"]);
    }
  }

  if((iframes/4)%2==0){
    for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
    tri[0].position=sf::Vector2f(player.x+player.hitboxx1,player.y+player.hitboxy1);
    tri[1].position=sf::Vector2f(player.x+player.hitboxx2,player.y+player.hitboxy1);
    tri[2].position=sf::Vector2f(player.x+player.hitboxx1,player.y+player.hitboxy2);
    tri[3].position=sf::Vector2f(player.x+player.hitboxx2,player.y+player.hitboxy2);
    player.anim++;
    if(player.anim>39)player.anim=0;
    int anim=0;
    if(gettinghit||hp<=0)anim=5;
    else if(player.xvelocity>2)anim=4;
    else if(!groundcheck){
      if(player.yvelocity<0)anim=3;
      else anim=6;
    }
    else
    switch(player.anim/10){
      case 0:case 2:anim=0;break;
      case 1:anim=1;break;
      case 3:anim=2;break;
    }
    tri[0].texCoords=sf::Vector2f((anim)*(player.hitboxx2-player.hitboxx1),0);
    tri[1].texCoords=sf::Vector2f((1+anim)*(player.hitboxx2-player.hitboxx1),0);
    tri[2].texCoords=sf::Vector2f((anim)*(player.hitboxx2-player.hitboxx1),player.hitboxy2-player.hitboxy1);
    tri[3].texCoords=sf::Vector2f((1+anim)*(player.hitboxx2-player.hitboxx1),player.hitboxy2-player.hitboxy1);
    rt.draw(tri,&texturemap["Player"]);
  }
  // for(int i=0;i<4;i++)tri[i].color=sf::Color::Green;
  // tri[0].position=sf::Vector2f(player.x+player.vertx,player.y+player.verty);
  // tri[1].position=sf::Vector2f(player.x+player.vertx2,player.y+player.verty);
  // tri[2].position=sf::Vector2f(player.x+player.vertx,player.y+player.verty2);
  // tri[3].position=sf::Vector2f(player.x+player.vertx2,player.y+player.verty2);
  // rt.draw(tri);

  tri.resize(5);
  tri.setPrimitiveType(sf::PrimitiveType::LineStrip);
  for(int i=0;i<5;i++)tri[i].color=sf::Color::Red;
  tri[0].position=sf::Vector2f(debugdelete.x+debugdelete.hitboxx1,debugdelete.y+debugdelete.hitboxy1);
  tri[1].position=sf::Vector2f(debugdelete.x+debugdelete.hitboxx2,debugdelete.y+debugdelete.hitboxy1);
  tri[2].position=sf::Vector2f(debugdelete.x+debugdelete.hitboxx2,debugdelete.y+debugdelete.hitboxy2);
  tri[3].position=sf::Vector2f(debugdelete.x+debugdelete.hitboxx1,debugdelete.y+debugdelete.hitboxy2);
  tri[4].position=sf::Vector2f(debugdelete.x+debugdelete.hitboxx1,debugdelete.y+debugdelete.hitboxy1);
  rt.draw(tri);


  for(int i=0;i<particledeque.size();i++)if(particledeque[i].frame<particledeque[i].len)rt.draw(particledeque[i]);

  tri.resize(4);
  tri.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
  for(int i=0;i<4;i++)tri[i].color=sf::Color::White;
  tri[0].position=sf::Vector2f(0,0);
  tri[1].position=sf::Vector2f(16,0);
  tri[2].position=sf::Vector2f(0,16);
  tri[3].position=sf::Vector2f(16,16);
  for(int i=0;i<3;i++){
    if(i<hp){
      uirt.draw(tri);
      for(int j=0;j<4;j++)tri[j].position+=sf::Vector2f(16,0);
    }
  }

  
}

void render(){
  rt.setView(view);
  window.clear();//원도우 클리어
  rt.clear();//랜더 텍스쳐 클리어
  uirt.clear(sf::Color::Transparent);//UI 랜더 텍스쳐 클리어
  if(mainmenu)mainmenurender();
  else gamelooprender();

  rt.display();
  sf::Sprite temp(rt.getTexture());
  window.draw(temp);

  uirt.display();
  temp.setTexture(uirt.getTexture(),true);
  window.draw(temp);
  
  window.display();
}

int init() {//프로그램 시작시 준비 시키는 함수(?)
  if(!texturemap["Player"].loadFromFile("assets/images/Maphie.png")||
  !texturemap["Background1"].loadFromFile("assets/images/Background.png")||
  !texturemap["Options"].loadFromFile("assets/images/Options.png")||
  !texturemap["Slash"].loadFromFile("assets/images/Slash.png"))return -1;//텍스쳐 파일 읽는 코드
  window.setFramerateLimit(60);//60fps로 제한
  window.setVerticalSyncEnabled(true);


  debugdelete.hitboxx1=0;debugdelete.hitboxx2=0;
  debugdelete.hitboxy1=0;debugdelete.hitboxy2=0;
  auto error = glz::read_file_json(currentmap,"assets/database/map.json",std::string{});
  if(error)return -1;

  std::sort(loadedmap.grounddeque.begin(),loadedmap.grounddeque.end());
  std::sort(loadedmap.obstacledeque.begin(),loadedmap.obstacledeque.end());

  setgame();

  return 0;
}

int main() {
  if(init()==-1)return 0;
  while (window.isOpen()) {
    windowset();
    input();
    if(debug)debugupdate();
    else update();
    render();
  }
  return 0;
}
