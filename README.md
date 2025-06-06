# DirectX11-Flight

DirectX 11 기반의 3D 비행 시뮬레이션입니다.  

‘월드 오브 워플레인’의 조작 방식을 참고해 비행기의 회전과 카메라 시스템을 구현해보고자 했으며, <br>
추후 기획에 따라 비행기 움직임은 더 개선해볼 계획입니다.

아직 미완성 상태의 프로젝트이며, 네트워크 서버도 간단한 소켓 프로그래밍만 구현되어 있습니다.

<br>

[유튜브 영상](https://youtu.be/LtHkrbhR0IQ?si=OpMynCfWETIsJcHL)

<br>

## 개발 개요

- **개발 인원** : 1인 개인 프로젝트
- **개발 언어** : C++, HLSL
- **개발 환경** : Visual Studio, DirectX11, PhysX

<br>

## 구현 목록

- **카메라**
  - 3인칭 카메라
  - 마우스 방향에 따른 시야 회전

<br>

- **비행**
  - 쿼터니언 기반의 회전 처리
  - 비행기는 카메라 방향을 따라가도록 보간(Slerp) 처리

<br>

- **이펙트**
  - 알파 블렌딩을 이용한 빌보드 렌더링
  - 노이즈 텍스쳐를 통한 연기가 움직이는 효과
  - 스프라이트 시트 기반 폭발 효과

<br>

- **충돌 처리**
  - PhysX 라이브러리를 활용한 충돌 처리
  - CollisionResponse, CollisionLayer, CollisionMatrix 개념 적용

<br>

- **기타**
  - 큐브맵 SkyBox
  - Heightmap 지형
  - 직교 투영을 통한 UI 렌더링
    
<br>
