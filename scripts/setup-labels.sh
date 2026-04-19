#!/bin/bash
# GitHub Issues のラベルセットアップスクリプト
# 使い方: bash scripts/setup-labels.sh

set -e

echo "Claude Task Executor 用ラベルをセットアップします..."

gh label create "claude:todo"        --color "0075ca" --description "Claudeに実行させるタスク（待機中）" 2>/dev/null \
  && echo "✅ claude:todo 作成" || echo "⏭️ claude:todo 既存"

gh label create "claude:in-progress" --color "e4e669" --description "Claudeが実行中" 2>/dev/null \
  && echo "✅ claude:in-progress 作成" || echo "⏭️ claude:in-progress 既存"

gh label create "claude:done"        --color "0e8a16" --description "Claude実行完了" 2>/dev/null \
  && echo "✅ claude:done 作成" || echo "⏭️ claude:done 既存"

echo ""
echo "セットアップ完了！"
echo ""
echo "タスクの登録方法:"
echo "  gh issue create --title 'タスク名' --body 'タスクの詳細' --label 'claude:todo'"
