
hunter_upload_password(
    # REPO_OWNER + REPO = https://github.com/ethereum/hunter-cache
    REPO_OWNER "ethereum"
    REPO "hunter-cache"

    # USERNAME = https://github.com/hunter-cache-bot
    USERNAME "hunter-cache-bot"

    # PASSWORD = GitHub token saved as a secure environment variable
    PASSWORD "$ENV{GITHUB_USER_PASSWORD}"
)
