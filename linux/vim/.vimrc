set nu      					    				"行号
set tabstop=4  				    					"tab 空格数
set shiftwidth=4
set mouse=a

"colo evening
"colo molokai
colo solarized
"set tags=/mnt/pos/Develop/tags  					"pos 项目的tags
set helplang=cn  									"vi帮助文档语言支持
set history=100
syntax on 											"语法高亮
"set t_Co=256                                        "支持256色
"中文支持
set encoding=utf-8 "中文显示
set fileencodings=utf-8,gb2312,gbk,gb18030
set termencoding=utf-8
set fileformats=unix 
"set fdm=indent                                      "代码缩进折叠
"set encoding=prc   								"对于pos代码无用
set hlsearch  										"搜索结果高亮匹配
"这告诉 Vim 当覆盖一个文件的时候保留一个备份。但 VMS 系统除外，因为 VMS 系统会
"自动产生备份文件。备份文件的名称是在原来的文件名上加上 "~" 字符。
set ruler  											"右下角显示光标位置
set showcmd 										"右下角显示未完成命令
set smartindent 									"新行使用旧行的缩
set incsearch 										"在输入部分查找模式时显示相应的匹配点。
set so=6  											"指定在距离边界多远的地方开始滚动文字。
set listchars=tab:>-,trail:- 						"TAB 会被显示成 ">---" ("-" 的个数不定) 而行尾多余的空白字符显示成 "-"
"set backup 										"设置默认打开文件时备份，以防丢失，且给vimdiff 命令使用对比修改记录





"ctags and cscope 
"taglist  windows

"map <C-F12> :!ctags -R --c++kinds=+p --fields=+iaS --extra=+q      "C-F12      在带根目录快捷生成tags文件

"使用vim的Tlist插件：提供源代码的结构化视图
let Tlist_Exit_OnlyWindow=1
let Tlist_Show_One_File=1

"cscope 配置
set cscopequickfix=s-,d-,i-,t-,e-     "在下面窗口显示

"OmniCppComplete插件   :输入时实时提供类或结构体的提示与补全
set nocp 
filetype plugin on 

"SuperTab 插件: 
let g:SuperTabDefaultCompletionType="context"

"MiniBufExplorer插件：多文件编辑
let g:miniBufExplMapWindowNavVim = 1   
let g:miniBufExplMapWindowNavArrows = 1   
let g:miniBufExplMapCTabSwitchBufs = 1   
let g:miniBufExplModSelTarget = 1  
let g:miniBufExplMoreThanOne=0 

"Winmanager插件：多窗口管理
let g:NERDTree_title="[NERDTree]"  
let g:winManagerWindowLayout="NERDTree|TagList"  
let g:winManagerWidth=32
  
  function! NERDTree_Start()  
      exec 'NERDTree' 
      endfunction  
        
        function! NERDTree_IsValid()  
            return 1  
            endfunction  
              
              nmap wm :WMToggle<CR> 
