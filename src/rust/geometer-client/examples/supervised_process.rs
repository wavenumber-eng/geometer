use std::env;
use std::io;
use std::path::PathBuf;
use std::process::Stdio;

use geometer_client::{
    GeometerClient, GeometerProcess, GeometerProcessController, GeometerProcessExit,
};
use tokio::process::{Child, Command};

struct OwnedChild {
    child: Child,
}

impl GeometerProcessController for OwnedChild {
    fn try_wait(&mut self) -> io::Result<Option<GeometerProcessExit>> {
        self.child.try_wait().map(|status| {
            status.map(|status| {
                GeometerProcessExit::new(status.success(), status.code().map(i64::from))
            })
        })
    }

    fn terminate(&mut self) -> io::Result<()> {
        match self.child.try_wait()? {
            Some(_) => Ok(()),
            None => self.child.start_kill(),
        }
    }
}

impl Drop for OwnedChild {
    fn drop(&mut self) {
        let _ = self.terminate();
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let executable = env::args_os()
        .nth(1)
        .map(PathBuf::from)
        .ok_or("usage: supervised_process GEOMETER_EXECUTABLE")?;
    let mut command = Command::new(executable);
    command
        .args(["serve", "--stdio"])
        .env_clear()
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .kill_on_drop(true);
    #[cfg(windows)]
    command.creation_flags(0x0800_0000); // CREATE_NO_WINDOW
    let mut child = command.spawn()?;
    let stdin = child
        .stdin
        .take()
        .ok_or("Geometer child stdin was not piped")?;
    let stdout = child
        .stdout
        .take()
        .ok_or("Geometer child stdout was not piped")?;
    let stderr = child
        .stderr
        .take()
        .ok_or("Geometer child stderr was not piped")?;
    let process = GeometerProcess::new(stdin, stdout, stderr, OwnedChild { child });
    let client = GeometerClient::from_process(
        process,
        "geometer-supervised-process-example",
        env!("CARGO_PKG_VERSION"),
    )
    .await?;
    println!("connected to Geometer {}", client.welcome().release_version);
    client.close().await?;
    Ok(())
}
