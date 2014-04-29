#======================================================================

source ("rvplot2-inc.R")
source ("lmlasso-inc.R")

fit.cpi.over.all.graphs <- function (Vars, Df.fit, Df.predict
                               , const.term=FALSE, nonneg=TRUE)
{
  response.var <- if (Vars$has.cycles) "Cycles" else "Time"
  Predictors <- setdiff (Vars$Predictors, "Instructions")
  
  response.true <- sprintf ("%s.true", response.var)
  Data.predicted <- NULL
  Predictions <- NULL

  Fits <- list ()
  Models <- list ()
  Predictors.all <- list ()
  for (arch in unique (Df.fit[["Architecture"]])) {
    for (alg in unique (Df.fit[["Algorithm"]])) {
      for (code in unique (Df.fit[["Implementation"]])) {
        cat (sprintf ("==> %s for %s on %s ...\n", code, alg, arch))
    
        # Choose subset of data to fit
        Include.fit <- with (Df.fit, Architecture == arch &
                                     Algorithm == alg &
                                     Implementation == code)
        Data.fit <- subset (Df.fit, Include.fit)

        # Fit!
        Fit <- fit.lm.lasso (response.var, Predictors, Data.fit
                             , const.term=const.term, nonneg=nonneg)

        # Use fitted model to predict totals
        Include.fit <- with (Df.predict, Architecture == arch &
                                         Algorithm == alg &
                                         Implementation == code)
        Cols.predict <- c (Vars$Index, Predictors, response.var)
        Data.predict <- subset (Df.predict, Include.fit)[, Cols.predict]
        Prediction <- predict.lm.lasso (Fit, Data.predict)

        # Aggregate
        mod.key <- get.file.suffix (arch, alg, code)
        Models[[mod.key]] <- Fit
        Predictions <- rbind.fill (Predictions, Prediction)
      } # for each code
    } # for each alg
  } # for each arch

  Fits$response.var <- response.var
  Fits$response.true <- response.true
  Fits$Predictions <- Predictions
  Fits$Models <- Models
  class (Fits) <- c ("cpi.lm.lasso", class (Fits))
  return (Fits)
}

#======================================================================
fit.cpi.one.per.graph <- function (Vars, Df.fit, Df.predict
                                   , const.term=FALSE, nonneg=TRUE)
{
  response.var <- if (Vars$has.cycles) "Cycles" else "Time"
  Predictors <- setdiff (Vars$Predictors, "Instructions")
  
  response.true <- sprintf ("%s.true", response.var)
  Data.predicted <- NULL
  Predictions <- NULL

  Fits <- list ()
  Models <- list ()
  Predictors.all <- list ()
  for (arch in unique (Df.fit[["Architecture"]])) {
    for (alg in unique (Df.fit[["Algorithm"]])) {
      for (code in unique (Df.fit[["Implementation"]])) {
        for (graph in unique (Df.fit[["Graph"]])) {
          cat (sprintf ("==> %s for %s on (%s, %s) ...\n", code, alg, arch, graph))
    
          # Choose subset of data to fit
          Include.fit <- with (Df.fit, Architecture == arch &
                                       Algorithm == alg &
                                       Implementation == code &
                                       Graph == graph)
          Data.fit <- subset (Df.fit, Include.fit)

          # Fit!
          Fit <- fit.lm.lasso (response.var, Predictors, Data.fit
                               , const.term=const.term, nonneg=nonneg)

          # Use fitted model to predict totals
          Include.fit <- with (Df.predict, Architecture == arch &
                                           Algorithm == alg &
                                           Implementation == code &
                                           Graph == graph)
          Cols.predict <- c (Vars$Index, Predictors, response.var)
          Data.predict <- subset (Df.predict, Include.fit)[, Cols.predict]
          Prediction <- predict.lm.lasso (Fit, Data.predict)

          # Aggregate
          mod.key <- sprintf ("%s--%s", get.file.suffix (arch, alg, code), graph)
          Models[[mod.key]] <- Fit
          Predictions <- rbind.fill (Predictions, Prediction)
        } # for each graph
      } # for each code
    } # for each alg
  } # for each arch

  Fits$response.var <- response.var
  Fits$response.true <- response.true
  Fits$Predictions <- Predictions
  Fits$Models <- Models
  class (Fits) <- c ("cpi.lm.lasso", class (Fits))
  return (Fits)
}

#======================================================================
decode.cpi.lm.model.key <- function (mod.key) {
  tags <- unlist (strsplit (mod.key, split="--"))
  return (c ("arch"=as.character (unlist (ARCHS.ALL.MAP[tags[1]]))
             , "alg"=as.character (unlist (ALGS.ALL.MAP[tags[2]]))
             , "code"=as.character (unlist (CODES.ALL.MAP[tags[3]]))
             , "graph"=tags[4]))
}

# Same as above, but returns a data frame
decode.cpi.lm.model.key.df <- function (mod.key) {
  meta.key <- decode.cpi.lm.model.key (mod.key)
  Meta.df <- data.frame (Architecture=as.character (meta.key["arch"])
                         , Algorithm=as.character (meta.key["alg"])
                         , Implementation=as.character (meta.key["code"]))
  if (!is.na (meta.key["graph"])) {
    Meta.df <- cbind (Meta.df, data.frame (Graph=as.character (meta.key["graph"])))
  }
  return (Meta.df)
}

#======================================================================

# Returns the greatest common set of predictors
get.gcp.cpi.lm.lasso <- function (Fits) {
  stopifnot ("cpi.lm.lasso" %in% class (Fits))
  Coefs <- NULL
  for (model in Fits$Models) {
    coefs <- coefs.nz.lm.lasso (model)
    if (is.null (Coefs)) { # first one
      Coefs <- names (coefs)
    } else {
      Coefs <- union (Coefs, names (coefs))
    }
  }
  return (Coefs)
}

#======================================================================
summary.cpi.lm.lasso <- function (Fits) {
  stopifnot ("cpi.lm.lasso" %in% class (Fits))
  for (mod.key in names (Fits$Models)) {
    Mod.key.df <- decode.cpi.lm.model.key.df (mod.key)

    model <- Fits$Models[[mod.key]]
    stopifnot ("lm.lasso" %in% class (model))

    cat (sprintf ("\n=== Model: %s ===\n", mod.key))
    print (Mod.key.df)
    cat ("\n")
    print (coefs.nz.lm.lasso (model))
  }
}

# eof
