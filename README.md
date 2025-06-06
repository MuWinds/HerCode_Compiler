# 🎀 HerCode Compiler: 编译你的专属代码世界! ✨

欢迎来到 HerCode 的世界！🎉 HerCode 是一款专为编程初学者（特别是女孩子们💖）设计的、简洁易懂的编程语言。<br>
它的目标是让你像写日记一样自然地表达创意，体验编程的乐趣与魅力。本项目是 HerCode 语言的编译器，使用 C 语言编写。

(P.S. 能想出来写这个的，家里请啥都没用了吧。。。感觉太适合拿来当成编译原理的作业了😉)

## 🌟 特性

* ✨ **简洁语法**: 像说话一样写代码，上手轻松。
* 🗣️ **友好输出**: 使用 `say` 关键字，轻松打印信息。
* 🔄 **C代码兼容**: 无缝嵌入C代码片段，扩展无限可能！
* 🛠️ **易于构建**: 支持 CMake 和 xmake 构建系统。

## 🚀 HerCode 快速上手

HerCode 的语法非常简单，让我们来看一个例子：

```hercode
# 这是一个 HerCode 程序示例

# 定义一个函数，打印欢迎信息
function say_hello:
	say "你好，欢迎来到 HerCode 的编程花园！🌷" #注释
	say "编程很美，也属于你。✨" #注释2
end

# 程序主入口
start:
    say_hello # 调用上面定义的函数
end
```

**基本语法元素:**

* `function <名称>:` : 定义一个函数。
* `end` : 结束一个函数定义结束 **（现已支持隐式结束）**。
* `say "内容"` : 打印一行文字到控制台。
* `start:` : 标志着程序执行的起点。
* `#` : HerCode 中的单行注释。

## ⚙️ 编译器使用方法

1. 编写你的 HerCode 代码，并保存为 `.hercode` 文件 (例如 `her.hercode`)。
2. 打开终端，使用以下命令编译：
    ```bash
    ./hercode_compiler.exe her.hercode --out=my_program.exe -r
    ```
    * 第一个参数：你的 HerCode 源文件。
    * 第二个参数：你希望生成的可执行文件的名称（可选，默认生成hercode.exe）。
	* 第三个参数：是否清除临时文件，-r表清除（可选）。  
3. 运行生成的可执行文件：
    ```bash
    ./my_program.exe
    ```

## 🧩 嵌入C代码指南

HerCode 编译器支持直接在 `.hercode` 文件中嵌入C语言代码！

* 在你的 `.hercode` 文件中，所有位于 `__c__ { ... }` 特殊标识 **内** 的内容都会被当作C代码。
* 这部分C代码的注释请使用C语言风格的 `//`。
* `__c__ { ... }` 特殊标识 **外** 的内容则必须遵循 HerCode 语法，注释使用 `#`。
* 编译器会自动包含常用的C标准库头文件，方便你直接调用C函数。

**示例 (`inline_c.hercode`):**

```hercode
# inline.hercode

function inline:
    say "HerCode调用C代码："
    __c__ {
        time_t rawtime;
        struct tm *info;
        #define BST (+1)
        #define CCT (+8)
        time(&rawtime);
        /* 获取 GMT 时间 */
        info = gmtime(&rawtime );

        printf("\t当前的世界时钟：\n");
        printf("\t中国时间 %2d:%02d\n", (info->tm_hour+CCT)%24, info->tm_min);
        printf("\t伦敦时间 %2d:%02d\n", (info->tm_hour+BST)%24, info->tm_min);
    }
    say "inline函数结束"

function add:
    say "HerCode准备执行C加法"
    __c__ {
        int x = 7, y = 8;
        printf("\tx + y = %d\n", x + y);
    }
    say "add函数结束"

function main:
    say "HerCode进入main"

    __c__ {
        // 可以直接写C代码
        printf("\n");
        printf("Hello! Her World\n");
        printf("编程很美，也属于你\n");
    }

    say " "
    inline
    add
    say " "
    
    say "main函数结束"
	# 不会报错，因为现已支持函数定义隐式结束。

start:
    main
end
```

## 🛠️ 如何构建编译器

你的项目包含 `CMakeLists.txt` 和 `xmake.lua`，这意味着你可以使用 CMake 或 xmake 来构建 HerCode 编译器：

### 使用 CMake

1. 创建并进入一个构建目录：
    ```bash
    mkdir build
    cd build
    ```
2. 运行 CMake 生成构建文件：
    ```bash
    cmake ..
    ```
3. 编译项目：
    ```bash
    cmake --build .
    # 或者在支持的平台上使用 make, ninja 等
    ```

### 使用 xmake

1. 在项目根目录下直接运行：
    ```bash
    xmake
    ```
2. 如果需要重新构建或清理：
    ```bash
    xmake rebuild
    xmake clean
    ```

## 💖 结语

希望你喜欢 HerCode！编码愉快，愿你的代码像诗一样优美！✍️<br>
如果你有任何想法、建议，或者发现了什么好玩/奇怪的 "特性" 😉，欢迎提出 Issue 或参与贡献！