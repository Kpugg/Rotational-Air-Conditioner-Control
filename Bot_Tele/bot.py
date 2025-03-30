import logging
import openai
import os
import warnings
from dotenv import load_dotenv
from telegram import Update, InlineKeyboardButton, InlineKeyboardMarkup
from telegram.ext import (
    Application,
    CommandHandler,
    MessageHandler,
    CallbackQueryHandler,
    filters,
    ContextTypes,
    ConversationHandler
)

# 🔇 Tắt cảnh báo cụ thể từ PTB
warnings.filterwarnings("ignore", category=UserWarning, module="telegram.ext._application")

# 🔒 Xác thực file .env
try:
    if not load_dotenv("bot.env"):
        raise ValueError("❌ Không tìm thấy file .env")

    REQUIRED_ENV_VARS = ["TELEGRAM_BOT_TOKEN", "OPENAI_API_KEY"]
    missing_vars = [var for var in REQUIRED_ENV_VARS if not os.getenv(var)]
    if missing_vars:
        raise ValueError(f"⚠️ Thiếu biến: {', '.join(missing_vars)}")

except Exception as e:
    logging.error(str(e))
    exit(1)

# Khởi tạo bot
BOT_TOKEN = os.getenv("TELEGRAM_BOT_TOKEN")
OPENAI_API_KEY = os.getenv("OPENAI_API_KEY")
openai.api_key = OPENAI_API_KEY

# Cấu hình logging nâng cao
logging.basicConfig(
    format="%(asctime)s - %(levelname)s - %(message)s",
    level=logging.WARNING,
    handlers=[
        logging.FileHandler("bot_debug.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

# Trạng thái hội thoại
ASK_AI = 1

async def start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Xử lý lệnh /start với menu inline"""
    keyboard = [
        [InlineKeyboardButton("💬 Chat với AI", callback_data="ask_ai")],
        [InlineKeyboardButton("🛠 Trợ giúp", callback_data="help")]
    ]
    await update.message.reply_text(
        "🤖 Chào mừng! Hãy chọn chức năng:",
        reply_markup=InlineKeyboardMarkup(keyboard)
    )

async def initiate_ai_chat(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Bắt đầu hội thoại AI"""
    query = update.callback_query
    await query.answer()
    await query.message.reply_text("📝 Vui lòng nhập câu hỏi của bạn:")
    return ASK_AI

async def process_ai_question(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Xử lý câu hỏi AI với kiểm tra bảo mật"""
    user_input = update.message.text.strip()

    # 🔒 Kiểm tra đầu vào nghiêm ngặt
    if not user_input or len(user_input) > 500:
        error_msg = "⚠️ Câu hỏi không hợp lệ!" if not user_input else "⚠️ Câu hỏi quá dài (tối đa 500 ký tự)"
        await update.message.reply_text(error_msg)
        return ASK_AI

    # 🛡 Lọc nội dung nhạy cảm
    SENSITIVE_KEYWORDS = ["mật khẩu", "thẻ tín dụng", "cvv", "riêng tư"]
    if any(kw in user_input.lower() for kw in SENSITIVE_KEYWORDS):
        await update.message.reply_text("⛔ Nội dung không được phép")
        return ConversationHandler.END

    try:
        # 🧠 Gọi OpenAI API với timeout
        response = openai.ChatCompletion.create(
            model="gpt-3.5-turbo-1106",
            messages=[{"role": "user", "content": user_input}],
            max_tokens=500,
            timeout=15  # Giới hạn thời gian phản hồi
        )
        ai_response = response.choices[0].message.content

        # 🔎 Lọc thông tin nhạy cảm trong phản hồi
        filtered_response = (
            ai_response.replace("API key", "[REDACTED]")
                      .replace("password", "[REDACTED]")
                      .replace("secret", "[REDACTED]")
        )
        await update.message.reply_text(f"🤖 {filtered_response}")

    except openai.error.RateLimitError:
        await update.message.reply_text("⚠️ Hệ thống đang quá tải, vui lòng thử lại sau 1 phút")
    except Exception as e:
        logger.error(f"Lỗi API: {str(e)}", exc_info=True)
        await update.message.reply_text("🔧 Đã xảy ra lỗi hệ thống")

    return ConversationHandler.END

async def cancel_conversation(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Hủy bỏ hội thoại hiện tại"""
    await update.message.reply_text("❌ Đã hủy tương tác hiện tại")
    return ConversationHandler.END

def setup_handlers(application: Application):
    """Cấu hình tất cả handlers"""
    # Conversation Handler cho AI
    ai_conv_handler = ConversationHandler(
        entry_points=[CallbackQueryHandler(initiate_ai_chat, pattern="^ask_ai$")],
        states={
            ASK_AI: [
                MessageHandler(filters.TEXT & ~filters.COMMAND, process_ai_question),
                CommandHandler("cancel", cancel_conversation)
            ]
        },
        fallbacks=[CommandHandler("cancel", cancel_conversation)],
        per_chat=True,
        per_user=True
    )

    # Đăng ký các handlers
    application.add_handler(CommandHandler("start", start))
    application.add_handler(ai_conv_handler)

def main():
    """Điểm khởi chạy chính"""
    application = Application.builder().token(BOT_TOKEN).build()

    # Cấu hình logging cho thư viện
    logging.getLogger("httpx").setLevel(logging.CRITICAL)
    logging.getLogger("telegram").setLevel(logging.WARNING)

    setup_handlers(application)
    application.run_polling()

if __name__ == "__main__":
    main()