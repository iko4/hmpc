use std::path::PathBuf;

use clap::{Args, Parser, Subcommand};
use hmpc_rs::net::PartyID;

mod cert;
mod sign;

macro_rules! verbose_info
{
    ($verbose:expr, $($str:expr),+) =>
    {
        if $verbose
        {
            log::info!($($str),+);
        }
    };
}

macro_rules! verbose_warn
{
    ($verbose:expr, $($str:expr),+) =>
    {
        if $verbose
        {
            log::warn!($($str),+);
        }
    };
}

pub(crate) use {verbose_info, verbose_warn};

#[derive(Parser, Debug)]
#[command(version, about)]
struct Cli
{
    #[command(flatten)]
    common: Common,
    #[command(subcommand)]
    command: Commands,
}

#[derive(Args, Debug)]
struct Common
{
    /// Config file
    #[arg(short, long)]
    config: Option<PathBuf>,
    /// Log (info) during execution
    #[arg(short, long)]
    verbose: bool,
}

#[derive(Subcommand, Debug)]
enum Commands
{
    /// Setup certificates
    Certificate(Cert),
}

#[derive(Args, Debug)]
struct Cert
{
    /// Party ID for which the certificate should be generated for
    id: Vec<PartyID>,
    /// Also output signing keys when generating certificate
    #[arg(short = 's', long)]
    and_signing_key: bool,
    /// Generate certificate even if some certificate files already exist
    #[arg(short, long)]
    force: bool,
}

fn main()
{
    let logger = env_logger::Builder::new()
        .filter_level(log::LevelFilter::Info)
        .parse_default_env()
        .build();
    let logger = Box::new(logger);
    log::set_boxed_logger(logger).expect("Could not set logging");
    log::set_max_level(log::STATIC_MAX_LEVEL);

    let cli = Cli::parse();
    verbose_info!(cli.common.verbose, "got arguments: {cli:?}");

    match cli.command
    {
        Commands::Certificate(cert) => cert::run(cli.common, cert),
    }
}
