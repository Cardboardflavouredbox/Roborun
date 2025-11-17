// Copyright 2025 Greenbox
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
sf::RenderTexture rt({160, 144});//랜더텍스쳐 (화면에 그릴거 있으면 여기)
sf::RenderTexture uirt({160,144});//
auto window = sf::RenderWindow(sf::VideoMode({160, 144}), "Roborun");
char right = 0, down = 0, up = 0, left = 0, confirm = 0, cancel = 0, select = 0,start = 0;//2=이번 프레임에 누름, 1=꾹 누르고 있음, 0=안 누르고 있음.
sf::Keyboard::Key rightkey = sf::Keyboard::Key::Right,
                  downkey = sf::Keyboard::Key::Down,
                  upkey = sf::Keyboard::Key::Up,
                  leftkey = sf::Keyboard::Key::Left,
                  confirmkey = sf::Keyboard::Key::Z,
                  cancelkey = sf::Keyboard::Key::X,
                  startkey = sf::Keyboard::Key::Enter,
                  selectkey = sf::Keyboard::Key::C;
sf::View view({0.f, 0.f}, {160.f, 144.f});
bool groundcheck=false;//땅에 닿았는지 여부
bool doublejump=true;//더블점프 가능 여부
bool dash=true;//대시 가능 여부
bool floating=false;//호버링 하고 있는지
char hp=3;//체력
char iframes=0;//무적프레임
char attacking=0;//공격 양수일시 가로, 음수일시 세로
std::unordered_map<std::string,sf::Texture> texturemap;//텍스쳐맵

float Lerp(float A, float B, float Alpha) {//선형 보간 함수
  return A * (1 - Alpha) + B * Alpha;
}


struct ground{//땅 클래스
    int x,y,x2,y2;
};
struct obstacle:ground{//장애물 클래스
    bool destroyable=false;
    bool bouncable=true;
    bool destroyed=false;
};
std::vector<ground> groundvector;//땅의 데이터가 저장된 벡터
std::vector<obstacle> obstaclevector;//장애물의 데이터가 저장된 벡터
struct entity{//엔티티 클래스
    int x=0,y=0;
    float xvelocity=0;//X 속도
    float yvelocity=0;//Y 속도
    int vertx=-8,verty=-1,vertx2=8,verty2=0;
    int hitboxx1=-8,hitboxy1=-16,hitboxx2=8,hitboxy2=0;
    int anim=0;
};
entity player;//플레이어


bool overlap(entity p,ground g)//엔티티와 땅이 겹치는지 확인하는 함수
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
  for(int i=0;i<groundvector.size();i++){
    if(overlap(player,groundvector[i])){
      groundcheck=true;
      while(overlap(player,groundvector[i]))player.y--;
      break;
      }
  }
  if(!groundcheck)player.y--;
}

void obstaclecollisioncheck(){//장애물 인식 함수
  for(int i=0;i<obstaclevector.size();i++){
    if(!obstaclevector[i].destroyed&&overlap(player,obstaclevector[i])){
      hp--;
      player.xvelocity=0;
      iframes=96;
      break;
      }
  }
}

void attackcollisioncheck(entity temp){//장애물 공격 인식 함수
  for(int i=0;i<obstaclevector.size();i++){
    if(!obstaclevector[i].destroyed&&overlap(temp,obstaclevector[i])){
      if(obstaclevector[i].destroyable)obstaclevector[i].destroyed=true;
      if(attacking>0){
        player.xvelocity=-2;
      }
      else if(obstaclevector[i].bouncable){
        player.yvelocity=-5;
        doublejump=true;
        dash=true;
      }
      break;
      }
  }
}




void update() {
  view.setCenter({Lerp(view.getCenter().x,float(player.x+64),0.5f),-64});
  
  groundcollisioncheck();

  if(groundcheck){player.yvelocity=0;doublejump=true;}//더블 점프 활성화 + 땅에 있을시에 y가속도 0으로 설정
  else player.yvelocity+=0.5f*(floating&&player.yvelocity>0?0.125f:1);//중력

  if(player.xvelocity>2)player.xvelocity-=0.5f;//X속도가 2보다 크다면 0.5씩 감소

  if(attacking>0)attacking--;
  else if(attacking<0)attacking++;

  if(player.xvelocity<2)player.xvelocity+=0.25f;
  if(player.xvelocity==2&&groundcheck)dash=true;
  if(confirm==2){//점프 키를 눌렀을 시
    floating=false;
    if(groundcheck){player.yvelocity=-7;groundcheck=false;}//땅에 닿았을 시 점프
    else if(doublejump){doublejump=false;player.yvelocity=-7;}//땅에 닿지 않았을시 더블 점프가 가능하다면 더블 점프
  }
  else if(confirm==1){//점프 키를 꾹 눌렀을 시
    floating=true;
  }
  else floating=false;
  if(select==2&&dash){//대시 가능하고 셀렉트 키(C)를 눌렀을 시
    floating=false;
    dash=false;
    player.xvelocity=10;
  }
  if(cancel==2){
    floating=false;
    if(groundcheck)attacking=2;
    else attacking=-2;
  }
  if(attacking>0){
    entity temp={player.x,player.y,0,0,0,0,0,0,4,-16,24,0};
    attackcollisioncheck(temp);
  }
  else if(attacking<0){
    entity temp={player.x,player.y,0,0,0,0,0,0,-8,0,8,20};
    attackcollisioncheck(temp);
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
}

void render() {//랜더 함수
  rt.setView(view);
  window.clear();//원도우 클리어
  rt.clear();//랜더 텍스쳐 클리어
  uirt.clear(sf::Color::Transparent);//UI 랜더 텍스쳐 클리어
  sf::VertexArray tri(sf::PrimitiveType::TriangleStrip, 4);
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
  for(int i=0;i<groundvector.size();i++){
    tri[0].position=sf::Vector2f(groundvector[i].x,groundvector[i].y);
    tri[1].position=sf::Vector2f(groundvector[i].x+groundvector[i].x2,groundvector[i].y);
    tri[2].position=sf::Vector2f(groundvector[i].x,groundvector[i].y+groundvector[i].y2);
    tri[3].position=sf::Vector2f(groundvector[i].x+groundvector[i].x2,groundvector[i].y+groundvector[i].y2);
    rt.draw(tri);
  }
  for(int i=0;i<4;i++)tri[i].color=sf::Color::Red;
  for(int i=0;i<obstaclevector.size();i++){
    tri[0].position=sf::Vector2f(obstaclevector[i].x,obstaclevector[i].y);
    tri[1].position=sf::Vector2f(obstaclevector[i].x+obstaclevector[i].x2,obstaclevector[i].y);
    tri[2].position=sf::Vector2f(obstaclevector[i].x,obstaclevector[i].y+obstaclevector[i].y2);
    tri[3].position=sf::Vector2f(obstaclevector[i].x+obstaclevector[i].x2,obstaclevector[i].y+obstaclevector[i].y2);
    rt.draw(tri);
  }
  
  if(attacking>0){
    for(int i=0;i<4;i++)tri[i].color=sf::Color::Cyan;
    tri[0].position=sf::Vector2f(player.x+4,player.y-16);
    tri[1].position=sf::Vector2f(player.x+24,player.y-16);
    tri[2].position=sf::Vector2f(player.x+4,player.y);
    tri[3].position=sf::Vector2f(player.x+24,player.y);
    rt.draw(tri);
  }
  else if(attacking<0){
    for(int i=0;i<4;i++)tri[i].color=sf::Color::Cyan;
    tri[0].position=sf::Vector2f(player.x-8,player.y);
    tri[1].position=sf::Vector2f(player.x+8,player.y);
    tri[2].position=sf::Vector2f(player.x-8,player.y+20);
    tri[3].position=sf::Vector2f(player.x+8,player.y+20);
    rt.draw(tri);
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
    if(player.xvelocity>2)anim=4;
    else if(!groundcheck){
      if(player.yvelocity<0)anim=3;
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

  rt.display();
  sf::Sprite temp(rt.getTexture());
  window.draw(temp);

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

  uirt.display();
  temp.setTexture(uirt.getTexture(),true);
  window.draw(temp);
  
  window.display();
}
int init() {//프로그램 시작시 준비 시키는 함수(?)
  if(!texturemap["Player"].loadFromFile("assets/images/Maphie.png")||
  !texturemap["Background1"].loadFromFile("assets/images/Background.png"))return -1;//텍스쳐 파일 읽는 코드
  view.setCenter({float(player.x+64),-64});
  window.setFramerateLimit(60);//60fps로 제한
  window.setVerticalSyncEnabled(true);

  groundvector.resize(3);//밟을 수 있는 땅의 개수
  groundvector[0]=ground{-32,0,4096,1};

  groundvector[1]=ground{128,-48,32,1};
  groundvector[2]=ground{192,-48,32,1};

  obstaclevector.resize(1);
  obstaclevector[0]=obstacle{512,-32,16,32,false,true};

  player.xvelocity=2;
  return 0;
}
int main() {
  if(init()==-1)return 0;
  while (window.isOpen()) {
    windowset();
    input();
    update();
    render();
  }
  return 0;
}
