# 🚀 IOCP 기반 MMO 서버

멀티스레드, IOCP, lock-free queue 및 TLS Memory pool 기반으로 설계된 고성능 MMO 서버입니다.
10000명 동시 접속을 목표로 select 기반 서버를 IOCP 기반으로 리팩토링하면서 성능을 개선하기 위해 진행했습니다.

---

## ⚙️ 기술 스택

- **Language** : C++14
- **Platform**: Windows (Visual Studio 2022)
- **Network**: IOCP (I/O Completion Port), Winsock2
- **Memory**: TLS 기반 메모리 풀, Generation 기반 ABA 방지
- **Concurrency**: Lock-Free MPSC Queue, Atomic 기반 세션 처리
- **Protocol**: Custom Binary Packet Protocol
- **Testing Tools**: Wireshark, netstat, GCP VM 환경

---

## 🧩 서버 구조
<pre> 📦 전체 스레드 구조 
  
  [AcceptThread] : 클라이언트 접속 수락, 세션 생성 
  [GQCSWorkerThread] : IOCP 이벤트 수신, recv/send 완료 처리 
  [LogicThread] : 패킷 로직 처리, AOI, 이동/좌표/동기화 
  [SendThread] : 송신 패킷 처리 및 WSASend 호출 
  [DeleteThread] : 세션 제거, 메모리 정리 
  
  📶 스레드 간 데이터 흐름 
  
          (Client) 
              ↓ 
          [AcceptThread]
              ↓ 
          [IOCP(GQCS)] ← WSASend, WSARecv 등록 
              ↓ 
          [GQCSWorkerThread] → packet → [LogicThread] → response → [SendThread] 
                                                      ↘
                                                        destroy → [DeleteThread] 


</pre>
- 각 스레드는 역할별로 분리되어 병목 최소화
- send 전용 스레드로 throughput 개선 추구
- 패킷은 커스텀 링버퍼 -> 파싱 -> 로직 처리 구조

---

## 💡 주요 구현 기능

- ✅IOCP 기반 비동기 소켓 처리 (WSARecv / WSASend)
- ✅flushSendRequestQueue로 안전한 세션 삭제 구조 설계
- ✅세션 및 플레이어 AOI 동기화 처리
- ✅lock-free MPSC queue 기반 cross-thread 메모리 관리
- ✅전용 SendThread 및 DeleteThread로 병목 최소화
- ✅ AOI (Area of Interest) 기반 캐릭터 위치 동기화

---

## 🔎 성능 테스트

| 항목 | select 서버 | IOCP 서버 |
|------|---------------|-------------|
| 테스트 환경 | LAN 7000 clients | LAN 10000 clients |
| CPU 사용률 | 17~20% | 재 테스트 중 |
| 전송 처리량 (패킷 평균 11 byte) | 2.1MB | 재 테스트 중 |

---

## 🧪 문제 상황 & 리팩토링

- GQCS 루프에 send 동시 처리 시 throughput 저하
  
  → send worker thread 분리 + 타이밍 조절로 개선
- MPSC queue의 self-loop 현상 발생
  
  → tail->next 타이밍 race 분석 및 generation control 적용
- WSASend 폭주로 인한 네트워크 포화
  
  → send batch 제한 + 시간 기준으로 flush 설계
- 동일 MPSC queue에 대해 서로 다른 스레드에서 consume 시도
  
  → MPSC queue를 추가하여 한 스레드에서만 consume 하도록 설계

---

## 🖥️ 실행 방법


```bash
# 빌드
Open in Visual Studio 2022 / C++14 / Windows 10

# 실행
1. 'IOCPTCPFighter.sln' 열기
2. 빌드 -> 실행 (F5)
3. 클라이언트는 없음.
```

---

## 📦 커스텀 패킷 구조

| 바이트 위치 | 필드 이름   | 크기(Byte) | 설명                       |
|-------------|-------------|------------|----------------------------|
| 0           | `length`    | 1          | 전체 패킷 길이             |
| 1           | `type`      | 1          | 패킷 종류 (ENUM)          |
| 2           | `checksum`  | 1          | 단순 오류 검출용   |
| 3~n         | `payload`   | N          | 실제 데이터 (ex. 이동 좌표 등) |

모든 패킷은 3byte header와 가변 길이 payload로 구성됩니다.

payload는 각 패킷의 type에 따라 구조체가 다르게 해석되며, 이 구조는 CPacket class를 통해 파싱됩니다.

---

## 🧠 회고 및 학습 포인트
- select 서버로는 7천명을 초과한 인원이 접속할 경우, 병목이 매우 심해졌기에 IOCP 구조를 학습하고 실험해보고 싶어서 이 프로젝트를 시작했습니다.
- select 기반 서버를 IOCP로 이전하면서 API 차이보다 **비동기 모델에 맞는 구조 설계**가 얼마나 중요한지를 경험했습니다.
- lock-free 구조를 만드는 것과 단일 MPSC queue에 대해 서로 다른 스레드들에서 consume하려고 했을 때, self-loop 문제가 발생해 이 부분에서 디버깅이 매우 어려웠습니다.
- 멀티스레드 환경에서의 메모리 재사용 문제를 풀기 위해서 lock-free MPSC queue, TLS Memory pool을 직접 설계하면서 low-level 시스템 감각을 익혔습니다.


---

## 🔗 참고
- 포트폴리오 Notion Page : https://www.notion.so/2021475751608044aba0c9561216bc11?source=copy_link
