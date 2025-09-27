본 보고서는 TizenRT 운영체제의 전력 관리(PM) 상태를 procfs를 통해 제공하는 pm_procfs.c 파일의 리팩토링 과정과 그 효과를 설명합니다. 복잡한 절차적 코드를 데이터 중심의 선언적 코드로 전환함으로써 가독성, 유지보수성, 확장성을 획기적으로 개선한 내용을 다룹니다.

1. 리팩토링 개요 및 목표
기존 pm_procfs.c 코드는 경로(relpath)를 분석하여 어떤 파일이나 디렉터리에 접근하는지를 판단하는 로직이 여러 if-else 문과 문자열 비교 함수로 복잡하게 얽혀 있었습니다. 이는 코드의 의도를 파악하기 어렵게 만들고, 새로운 기능을 추가할 때 오류를 유발할 가능성이 높았습니다.

개선 목표:

가독성 향상: 코드의 구조를 한눈에 파악할 수 있도록 개선합니다.

유지보수성 증대: 새로운 procfs 경로를 추가하거나 수정할 때, 최소한의 코드 변경으로 가능하도록 구조를 변경합니다.

확장성 확보: 향후 더 복잡한 procfs 인터페이스가 필요할 경우에도 유연하게 대처할 수 있는 기반을 마련합니다.

2. 핵심 개선 사항
2.1. 절차적 코드에서 선언적 코드로의 전환
가장 큰 변화는 경로를 해석하는 방식을 근본적으로 바꾼 것입니다.

수정 전 (Before): 복잡하고 오류에 취약한 구조
기존 코드는 경로를 해석하는 로직이 하나의 거대한 함수(power_find_dirref)에 집중되어, 여러 문제점을 안고 있었습니다.

1. 높은 논리적 복잡도:
경로의 각 부분을 확인하기 위해 if-else 문이 깊게 중첩되어 코드의 흐름을 파악하기 매우 어려웠습니다. 예를 들어, /proc/power/domains/cpu/info 경로를 확인하는 로직은 다음과 같은 다중 분기를 거쳐야만 했습니다.

/* 기존 power_find_dirref 함수의 일부 */
static int power_find_dirref(FAR const char *relpath, FAR struct power_dir_s *dir)
{
    const char *str = relpath;

    /* Function to check path is starting with given name */
    int checkStart(const char *name, bool isDirectory) {
        // ... strncmp, str 포인터 조작 ...
    }

    /* "power/" 확인 */
    if (checkStart(POWER, true) == OK) {
        if (str[0] == '\0') { /* ... */ return OK; }

        /* "domains/" 확인 */
        if (checkStart(POWER_DOMAINS, true) == OK) {
            if (str[0] == '\0') { /* ... */ return OK; }

            /* for 루프를 돌며 등록된 모든 도메인 이름을 문자열 비교 */
            for (entry = dq_peek(&g_pmglobals.domains); entry != NULL; entry = dq_next(entry)) {
                domain = (FAR struct pm_domain_s *)entry;
                /* "cpu/" 확인 */
                if (checkStart(domain->name, true) == OK) {
                    /* "info" 확인 */
                    if (checkStart(POWER_INFO, false) == OK) {
                        /* 최종 매칭 성공 */
                        return OK;
                    }
                }
            }
        }
    }
    return -ENOENT;
}

이러한 구조는 새로운 경로를 추가할 때마다 더 깊은 if 문을 추가해야 하므로 유지보수가 극도로 어려워집니다.

2. 수동 상태 관리의 위험성:
코드는 파일 시스템의 상태를 level, index, nentries와 같은 숫자 변수를 통해 수동으로 관리했습니다.

/* 예시: power/domains 경로에 진입했을 때의 코드 */
if (checkStart(POWER_DOMAINS, true) == OK) {
    if (str[0] == '\0') {
        dir->base.level = POWER_LEVEL_2;
        dir->base.index = 0;
        dir->base.nentries = g_pmglobals.ndomains + 1; /* +1 for "info" file */
        dir->dtype = DTYPE_DIRECTORY;
        return OK;
    }
}

만약 power/domains 디렉터리에 새로운 정적 파일(예: summary)을 추가하려면, 개발자는 nentries 값을 +2로 변경하는 것을 잊지 말아야 합니다. 이런 "매직 넘버" 관리는 실수를 유발하기 쉬우며, 코드와 실제 디렉터리 구조 간의 불일치를 초래할 수 있습니다.

3. 비효율적인 readdir 구현:
power_readdir 함수는 디렉터리 내용을 읽을 때마다 매번 도메인 목록의 처음부터 순회했습니다. 예를 들어 5번째 도메인 이름을 읽기 위해서는 연결 리스트(linked list)를 5번 순회해야 했습니다.

/* 기존 power_readdir 함수 일부 */
case POWER_LEVEL_2:
    if (index == g_pmglobals.ndomains) { /* ... */ }
    else {
        /* 매번 dq_peek로 리스트의 처음부터 시작 */
        entry = dq_peek(&g_pmglobals.domains);
        /* index번째 항목을 찾기 위해 for 루프 실행 */
        for (int i = 0; i < index && entry != NULL; i++) {
            entry = dq_next(entry);
        }
        // ...
    }
    break;

도메인 수가 많아질 경우, 디렉터리 목록을 조회하는 성능이 크게 저하되는 O(N²) 문제를 야기했습니다.

수정 후 (After): 선언적이고 명확한 구조
/proc/power의 모든 파일 및 디렉터리 구조를 템플릿 트리(Template Tree) 라는 정적 데이터로 미리 정의했습니다. 실제 코드는 이 데이터 구조를 순회하여 경로를 찾는 단순하고 명확한 방식으로 변경되었습니다.

/* Static Path Template Tree */
/* 이 데이터 구조는 procfs 레이아웃을 선언적으로 정의합니다. */
static const struct power_path_template_s g_power_root_path_list[] = {
    {
        .name       = "power/domains/",
        .dtype      = DTYPE_DIRECTORY,
        .read       = NULL,
        .readdir    = power_readdir_domains,
        .children   = g_power_domain_path_list
    },
    {
        .name       = "power/state",
        .dtype      = DTYPE_FILE,
        .read       = power_read_state,
        .children   = NULL,
    },
    { .name = NULL } /* Sentinel */
};

구현 의도: 코드 로직에서 파일 시스템의 "구조"를 분리하여, 구조는 데이터가 정의하고 코드는 그 데이터를 "해석"하는 역할만 하도록 만들었습니다. 이로써 "어떻게(How)" 찾을 것인가에 대한 복잡한 설명 대신 "무엇(What)" 이 있는지에 대한 명확한 명세가 코드의 중심이 되었습니다.

3. 적용된 디자인 패턴
3.1. 템플릿 메소드 패턴 (Template Method Pattern)
VFS의 핵심 함수인 power_read가 템플릿 메소드 역할을 합니다. 이 함수는 파일 읽기의 전체적인 흐름(버퍼 관리, 오프셋 처리 등)을 정의하고, 실제 파일 내용을 생성하는 세부적인 단계는 각 템플릿에 정의된 read 함수에게 콜백(Callback) 함수 readprint를 전달하여 위임합니다.

템플릿 메소드: power_read (전체 알고리즘 뼈대 제공)

구체적인 구현: power_read_state, power_read_domains_info 등 (세부 내용 생성)

/* power_read는 모든 읽기 요청의 공통 로직을 처리하는 템플릿 역할을 합니다. */
static ssize_t power_read(FAR struct file *filep, FAR char *buffer, size_t buflen)
{
    FAR struct power_file_s *priv = (FAR struct power_file_s *)filep->f_priv;
    size_t totalsize = 0;
    int last_read = priv->offset;

    /* 1. 복잡한 버퍼 관리 로직을 처리하는 콜백 함수 'readprint' 정의 */
    void readprint(const char *format, ...) {
        // ... vsnprintf, 오프셋(last_read) 처리, totalsize 업데이트 등 ...
    }

    /* 2. 세부 구현 호출: readprint 콜백을 전달하여 내용 생성을 위임 */
    /* power_read_state 등은 이제 버퍼를 직접 다루지 않고 readprint만 호출하면 됨 */
    priv->path_priv.template->read(priv, readprint);

    /* 3. 공통 마무리 단계 (총 사이즈 반환) */
    filep->f_pos += totalsize;
    return totalsize;
}

3.2. 컴포지트 패턴 (Composite Pattern)
power_path_template_s 구조체는 .children 포인터를 통해 자기 자신과 같은 타입의 객체(자식 노드)를 가리킵니다. 이를 통해 개별 노드(파일)와 노드의 집합(디렉터리)을 동일한 방식으로 다룰 수 있는 트리 구조가 형성됩니다.

/* 템플릿 구조체 정의 */
struct power_path_template_s {
    const char *name;
    uint8_t dtype;
    void (*read)(/* ... */);
    int (*readdir)(/* ... */);
    /* 자식 노드를 가리켜 트리 구조 형성 */
    const struct power_path_template_s *children;
};

power_find_best_match 함수가 이 트리 구조를 재귀적으로 순회하는 것이 바로 컴포지트 패턴의 활용 예입니다.

graph TD
    A[power_template_root] --> B(g_power_root_path_list);
    B --> C["power/domains/"];
    B --> D["power/state"];
    C --> E(g_power_domain_path_list);
    E --> F["power/domains/info"];
    E --> G["power/domains/%s/"];
    G --> H(power_domain_dynamic_path_list);
    H --> I["power/domains/%s/info"];

    subgraph "컴포지트(디렉터리)"
        A; C; G;
    end
    subgraph "리프(파일)"
        D; F; I;
    end

3.3. 스트래티지 패턴 (Strategy Pattern) 개념의 활용
각 경로 템플릿에 read와 readdir라는 함수 포인터를 두어, 파일의 내용을 읽거나 디렉터리의 목록을 조회하는 **전략(Strategy)**을 동적으로 선택할 수 있게 했습니다. power_find_dirref 함수가 경로에 맞는 템플릿을 찾으면, 그 템플릿에 정의된 함수(전략)가 실행됩니다.

/* power_readdir 함수는 찾은 템플릿의 readdir 함수(전략)를 호출하는 역할만 수행 */
static int power_readdir(struct fs_dirent_s *dir)
{
    FAR struct power_dir_s *powerdir = dir->u.procfs;

    if (powerdir && powerdir->path_priv.template && powerdir->path_priv.template->readdir) {
        /* 각 디렉터리에 맞는 readdir 전략을 실행 */
        return powerdir->path_priv.template->readdir(dir);
    }
    return -ENOSYS;
}

구현 의도: 경로마다 달라지는 read/readdir 동작을 캡슐화하고 교체 가능하게 만들어, if-else나 switch 문 없이도 유연하게 동작을 확장할 수 있습니다.

4. 코드 품질 및 기대 효과
4.1. 복잡도 감소
power_find_dirref 함수의 사이클로매틱 복잡도(Cyclomatic Complexity)가 현저히 감소했습니다. 복잡한 분기문이 제거되고, 데이터를 순회하는 단순하고 예측 가능한 로직으로 대체되었습니다.

4.2. 명확성과 응집도 향상
파일 시스템의 구조, 각 경로의 속성(파일/디렉터리), 그리고 해당 경로에 대한 동작(read/readdir 함수)이 power_path_template_s라는 단일 데이터 구조 안에 함께 정의되어 코드의 응집도가 높아졌습니다. 또한, power_path_priv_s 구조체는 경로 탐색 결과를 깔끔하게 캡슐화합니다.

4.3. 기대 효과
유지보수 비용 감소:

새로운 개발자가 코드의 구조를 이해하는 데 걸리는 시간이 단축됩니다.

/proc/power/에 새로운 항목을 추가할 때, 템플릿 배열에 새 구조체를 추가하는 것만으로 대부분의 작업이 완료되므로, 변경이 쉽고 안전합니다.

안정성 향상:

복잡한 로직의 제거로 잠재적인 버그(예: 동기화 락 해제 누락) 발생 가능성이 줄어듭니다.

sscanf를 활용한 경로 매칭은 기존 방식보다 더 안전하고 명시적입니다.

확장성 증대:

향후 더 깊고 복잡한 procfs 구조가 필요하더라도, 템플릿 트리를 확장하는 것만으로 유연하게 대응할 수 있습니다.

5. 결론
이번 리팩토링은 단순한 코드 정리 수준을 넘어, 시스템의 복잡성을 관리하는 방식을 근본적으로 개선한 성공적인 사례입니다. 데이터 중심의 선언적 설계와 적절한 디자인 패턴의 적용을 통해, pm_procfs.c 코드는 더 안정적이고, 이해하기 쉬우며, 미래의 요구사항 변화에 쉽게 적응할 수 있는 고품질 코드로 거듭났습니다.