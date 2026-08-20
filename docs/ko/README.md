# Weqi

**Weqi**는 현대적이고 깔끔한 오픈소스 데스크톱 체스 애플리케이션입니다. 모든 체스 규칙은 로컬 C++ 엔진에 구현되어 있으며, 인간 대 인간, 인간 대 AI, AI 대 AI, 기보 재생을 지원합니다.

> **이 문서를 원하는 언어로 읽으세요:**
> [English](../../README.md) · [简体中文](../zh-CN/README.md) · [繁體中文](../zh-TW/README.md) · [日本語](../ja/README.md) · [Español](../es/README.md) · [Українська](../uk/README.md)

---

## 기능

- **네 가지 게임 모드**
  - **인간 대 인간** — 같은 기기에서 두 명이 대국.
  - **인간 대 AI** — 선택한 AI 제공업체와 대국.
  - **AI 대 AI** — 두 AI가 자동으로 대국 (당신의 전적에 포함되지 않음).
  - **기보 재생** — 과거 대국을 다시 보기.
- **완전한 체스 규칙**이 로컬 C++로 구현됨 (합법 수 검증, 체크, 체크메이트, 스테일메이트, 캐슬링, 앙파상, 프로모션 등).
- **플레이어 프로필 및 레이팅** — 레이팅, 최고 레이팅을 추적하고 플레이어 이름을 편집할 수 있습니다.
- **전적 통계** — 대국 수, 승률, 승, 무, 패, 최고/현재 연승, 최고 레이팅.
- **대국 기록** — 날짜, 모드, 상대, 결과, 레이팅 변동이 포함된 과거 대국을 탐색.
- **미완료 대국 계속하기** — 진행 중인 대국을 언제든지 재개.
- **AI 제공업체** — AI 제공업체 추가·편집·삭제 (이름, 유형, Base URL, API 키, 모델). API 키는 시스템 사용자 데이터 디렉터리에만 비공개로 저장되며 프로젝트에는 절대 기록되지 않습니다.
- **AI 채팅** — AI가 자신의 수에 대해 짧은 코멘트를 표시하며, 메시지는 UI 언어를 따릅니다.
- **국제화(i18n)** — 7가지 UI 언어: 간체 중국어, 번체 중국어, 영어, 일본어, 스페인어, 우크라이나어, 한국어. 재시작 없이 런타임에 언어를 전환할 수 있습니다.

## 기술 스택

- **C++17**
- **Qt 6** (Widgets)
- **CMake**

## 디렉터리 구조

```
weqi/
├── CMakeLists.txt
├── README.md
├── ai_adapter/          # Python AI 어댑터 (언어 감지, 재시도/조정)
│   ├── main.py
│   ├── parser.py
│   └── providers/
│       └── openai_compatible.py
├── dist/                # 배포용 디렉터리 (컴파일된 바이너리 + 패키징 스크립트)
├── resources/           # Qt 리소스 파일
├── scripts/             # 보조 스크립트
├── src/
│   ├── main.cpp         # 프로그램 진입점
│   ├── MainWindow.h/.cpp
│   ├── GameController.h/.cpp
│   ├── ChessBoard.h/.cpp
│   ├── ChessPiece.h/.cpp
│   ├── ai/              # AI 매니저
│   ├── data/            # 설정, 언어, 프로필, 전적, 기록 매니저
│   └── pages/           # 홈, 새 대국, 대국, 인간-AI, AI-AI, 기록, 설정, 정보
├── tests/               # C++ 테스트
├── tools/               # 개발 도구
└── translations/        # Qt .ts 번역 파일 (7개 언어)
```

## 빌드

### 의존성

- CMake ≥ 3.16
- Qt 6 (≥ 6.2, Widgets 모듈 포함)
- C++17 지원 컴파일러 (GCC / Clang)
- Python 3 (AI 어댑터용)

### 빌드 단계

```bash
# 1. 설정
cmake -S . -B build

# 2. 빌드
cmake --build build -j

# 3. 실행
./build/Weqi
```

또는 한 줄로:

```bash
cmake -S . -B build && cmake --build build -j && ./build/Weqi
```

### 번역

번역 파일은 `translations/`에 있으며 Qt의 `qt_add_translations`를 통해 바이너리에 컴파일됩니다. `tr()` 문자열을 변경한 후 번역을 업데이트하세요:

```bash
# .ts 파일 업데이트 (새 문자열 추출)
lupdate src -ts translations/weqi_<lang>.ts

# .qm 파일 릴리스
lrelease translations/weqi_<lang>.ts
```

## 패키징

`dist/` 디렉터리에는 컴파일된 바이너리와 Python AI 어댑터가 포함되어 있으며, 설치 프로그램으로 패키징할 준비가 되어 있습니다.

```
dist/
├── Weqi                  # 컴파일된 C++ 바이너리 (Qt6 Widgets, 7개 언어 내장)
├── ai_adapter/           # Python AI 어댑터
├── package-deb.sh        # .deb 빌드 (Linux)
├── package-rpm.sh        # .rpm 빌드 (Linux, Fedora/RHEL/openSUSE)
├── weqi.spec             # RPM spec 파일
├── package-exe.sh        # .exe 빌드 (Windows, Windows에서 실행)
├── package-appimage.sh   # .AppImage 빌드 (Linux, 모든 배포판)
├── weqi.desktop          # AppImage 데스크톱 항목
├── weqi.png              # AppImage 아이콘 (512×512)
└── README.md
```

- **.deb**: `cd dist && ./package-deb.sh` → `weqi_0.1.0_amd64.deb`
- **.rpm**: `cd dist && ./package-rpm.sh` → `weqi-0.1.0-1.fc44.x86_64.rpm`
- **.exe**: Windows에서 `./package-exe.sh` 실행 → `weqi-win/` 디렉터리 (`windeployqt` 사용)
- **.AppImage**: `cd dist && ./package-appimage.sh` → `Weqi-0.1.0-x86_64.AppImage` (단일 파일, 설치 불필요)

바이너리는 실행 파일 기준 상대 경로로 `ai_adapter/main.py`를 찾습니다. 패키지로 설치한 경우 `/usr/share/weqi/ai_adapter/main.py`에서 로드합니다.

## 사용 방법

- **기물 선택**: 아무 기물이나 왼쪽 클릭하여 강조 표시합니다.
- **기물 이동**: 선택 후 목표 칸을 클릭합니다. 로컬 엔진이 합법 수를 검증합니다.
- **무르기**: «무르기» 버튼을 사용하여 한 수를 되돌립니다.
- **새 대국**: 대국 페이지 또는 홈 화면에서 새 대국을 시작합니다.
- **AI 대 AI**: 두 AI 제공업체를 선택하고 «시작 / 일시정지 / 재개 / 중지»로 자동 대국을 제어합니다.
- **언어 변경**: «설정 → 언어»를 열고 지원되는 7개 언어 중 하나를 선택합니다 (또는 «시스템 따르기»).

## AI 모델 추가

AI와 대국하려면 먼저 «설정 → AI Providers»에서 AI 제공업체를 추가해야 합니다. 각 제공업체에는 5개의 필드가 있습니다:

| 필드 | 입력 내용 |
| --- | --- |
| **이름** | 직접 정하는 표시 이름. 예: `DeepSeek`, `Qwen`, `豆包`. |
| **제공업체 유형** | 아래 모델들은 모두 OpenAI 호환 API를 사용하므로 `OpenAI Compatible`을 입력합니다. |
| **Base URL** | 제공업체의 API 엔드포인트 (아래 표 참조). |
| **API Key** | 제공업체 콘솔에서 발급한 비밀 키. 로컬 사용자 데이터 디렉터리에만 저장되며 프로젝트에는 기록되지 않습니다. |
| **모델** | 정확한 모델 이름 (아래 표 참조). |

### 주요 모델 권장 설정

아래 제공업체들은 모두 OpenAI 호환 엔드포인트를 제공하므로 «제공업체 유형»은 모두 `OpenAI Compatible`을 입력합니다.

| 제공업체 | Base URL | 모델 (예시) | API Key 발급처 |
| --- | --- | --- | --- |
| **DeepSeek** | `https://api.deepseek.com/v1` | `deepseek-chat` | platform.deepseek.com |
| **Qwen (통의천문)** | `https://dashscope.aliyuncs.com/compatible-mode/v1` | `qwen-plus` | bailian.console.aliyun.com |
| **豆包 (Doubao)** | `https://ark.cn-beijing.volces.com/api/v3` | `doubao-1-5-pro-32k-250115` | console.volcengine.com/ark |
| **ChatGPT (OpenAI)** | `https://api.openai.com/v1` | `gpt-4o` | platform.openai.com |
| **元宝 (Tencent Hunyuan)** | `https://api.hunyuan.cloud.tencent.com/v1` | `hunyuan-turbo` | console.cloud.tencent.com/hunyuan |
| **Gemini (Google)** | `https://generativelanguage.googleapis.com/v1beta/openai` | `gemini-2.0-flash` | aistudio.google.com |
| **MiniMax** | `https://api.minimax.chat/v1` | `MiniMax-Text-01` | platform.minimaxi.com |
| **Kimi (Moonshot)** | `https://api.moonshot.cn/v1` | `moonshot-v1-8k` | platform.moonshot.cn |
| **Mimo (Xiaomi)** | `https://api.mimo.ai/v1` | `mimo-1` | platform.mimo.ai |
| **Claude (Anthropic)** | `https://api.anthropic.com/v1` | `claude-sonnet-4-20250514` | console.anthropic.com |
| **Grok (xAI)** | `https://api.x.ai/v1` | `grok-2-latest` | console.x.ai |

> **참고**: 모델 이름과 엔드포인트는 자주 변경됩니다. 최신 모델 ID와 Base URL은 각 제공업체의 공식 문서를 확인하세요. «이름» 필드는 직접 정하는 라벨일 뿐이며 실제 호출되는 API에는 영향을 주지 않습니다.

## 데이터 저장

- **설정 및 프로필**: 시스템 사용자 데이터 디렉터리에 저장 (Linux에서는 `~/.local/share/Weqi/` 등).
- **AI 제공업체 설정**: 사용자 데이터 디렉터리에 별도로 저장되며 프로젝트에는 기록되지 않습니다.
- **대국 기록 및 저장된 대국**: 사용자 데이터 디렉터리에 저장.

## 라이선스

오픈소스. 자세한 내용은 저장소를 참조하세요.
