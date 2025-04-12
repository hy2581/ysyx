#!/bin/bash

# usage: 定义一个名为addenv的函数，用于添加环境变量，接受两个参数：环境变量名和路径。
function addenv() {
  sed -i -e "/^export $1=.*/d" ~/.bashrc
  echo -e "\nexport $1=`readlink -e $2`" >> ~/.bashrc
#   在 .bashrc 文件末尾添加一行 export 变量名=绝对路径，例如：
#   export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
  echo "By default this script will add environment variables into ~/.bashrc."
  echo "After that, please run 'source ~/.bashrc' to let these variables take effect."
  echo "If you use shell other than bash, please add these environment variables manually."
}

# usage: init repo branch directory trace [env]
# trace = true|false
# 定义一个名为init的函数，用于初始化项目仓库，接受5个参数：GitHub仓库、分支名、目标目录、是否跟踪和可选的环境变量名。
function init() {
  if [ -d $3 ]; then
    echo "$3 is already initialized, skipping..."
    return
  fi

  while [ ! -d $3 ]; do
    git clone -b $2 git@github.com:$1.git $3
  done
  log="$1 `cd $3 && git log --oneline --no-abbrev-commit -n1`"$'\n'

  if [ $4 == "true" ] ; then
    rm -rf $3/.git
    git add -A $3
    git commit -am "$1 $2 initialized"$'\n\n'"$log"
  else
    sed -i -e "/^\/$3/d" .gitignore
    echo "/$3" >> .gitignore
    git add -A .gitignore
    git commit --no-verify --allow-empty -am "$1 $2 initialized without tracing"$'\n\n'"$log"
  fi

  if [ $5 ] ; then
    addenv $5 $3
  fi
}

case $1 in
  nemu)
    init NJU-ProjectN/nemu ics2024 nemu true NEMU_HOME
    ;;
  abstract-machine)
    init NJU-ProjectN/abstract-machine ics2024 abstract-machine true AM_HOME
    init NJU-ProjectN/fceux-am ics2021 fceux-am false
    ;;
  am-kernels)
    init NJU-ProjectN/am-kernels ics2021 am-kernels false
    ;;
  nanos-lite)
    init NJU-ProjectN/nanos-lite ics2021 nanos-lite true
    ;;
  navy-apps)
    init NJU-ProjectN/navy-apps ics2024 navy-apps true NAVY_HOME
    ;;
  *)
    echo "Invalid input..."
    exit
    ;;
esac
